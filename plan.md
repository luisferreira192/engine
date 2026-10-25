# Performance plan

Goal: take the current engine (box/circle bodies, mass, uniform-grid broad phase,
impulse resolution) to a concrete performance target, which we'll pick once we
have trustworthy numbers.

1. **Profiler first:** a simple but solid measurement setup whose numbers we can
   trust and reproduce.
2. **Then scale:** single-thread groundwork → multithreading → GPU.

Every optimisation step below is judged by the Phase 1 tooling. If it doesn't
show up in the numbers, it didn't happen.

---

## 0. Where we are (measured 2026-10-25, stress scenario, 3000 bodies, 400 cells)

| phase          | current build (no optimisation) | Release `-O3` |
|----------------|---------------------------------|---------------|
| frame          | 25.4 ms                         | 18.6 ms       |
| collision      | 16.4 ms                         | 9.1 ms        |
| draw + EndDraw | 7.9 ms                          | 9.5 ms        |
| grid           | 0.52 ms                         | 0.13 ms       |
| move + wall    | 0.28 ms                         | 0.03 ms       |

What this tells us:

- **Every number recorded so far is from an unoptimised build.** `CMAKE_BUILD_TYPE`
  was empty, so the code ran with no optimisation and the 2026-09-26 analysis
  measured that. In Release, `move`/`wall`/`grid` get 4–13× faster but collision
  only 1.8×, which suggests it's memory-bound rather than compute-bound.
- **Rendering is half the optimised frame, and it runs on the CPU.** The VM has no
  GPU acceleration (see §1), so drawing competes with physics for the same cores.
- **Prime suspect:** `CollisionSystem::tested_pairs_` reserves n(n−1)/2 buckets
  (≈4.5M for 3000 bodies, ~36 MB) and `clear()`s them every frame. That's O(n²)
  memory and a full-table wipe per step. Phase 1 should confirm or rule this out
  before we touch it.

---

## 1. Environment: the VM

| | value | consequence |
|---|---|---|
| Hypervisor | VirtualBox (KVM paravirt) | timing noise from the host; need repeats + percentiles |
| CPU | 8 vCPU of a Ryzen 9 5900HS (8C/16T host), AVX2 + FMA | threads scale to ~8; vCPUs may map to SMT siblings, so scaling can flatten early |
| RAM | 7.8 GB | fine; the O(n²) pair set is a wall long before RAM is (10k bodies ≈ 400 MB) |
| GPU | VMware SVGA II, software OpenGL 4.1, `Accelerated: no` | **no RTX 3060M access**, no compute shaders (need GL 4.3) |
| HW counters | `perf_event_paranoid = 3`, no PMU exposed | no cache-miss / IPC data in the VM; the profiler has to time code itself |
| Clock | TSC clocksource | `steady_clock` is cheap and accurate |

**Multithreading in the VM is fine.** Relative results (speedup curves, which
phase dominates) carry over to bare metal; absolute numbers won't. Rules:
benchmark headless (otherwise software rendering steals cores), close heavy apps
on the host, repeat runs, and confirm key results on bare metal once.

**The GPU phase can't happen in this VM.** VirtualBox has no practical GPU
passthrough, and a laptop 3060M behind Optimus is a hard case even for
hypervisors that support it.

**Decision: the host is Windows with WSL2 already set up; dual-boot Debian is
possible.**

| where | use it for | limits |
|---|---|---|
| this VM | Phases 1–3 development, headless trials, relative comparisons | software rendering, noisy timing, no GPU, no hardware counters |
| WSL2 | CUDA experiments, headless | rendering goes through WSLg's translation layer, so visual-performance numbers aren't representative |
| **dual-boot Debian** (recommended for final numbers) | the reference machine: bare-metal timings, `perf` hardware counters, native NVIDIA driver (OpenGL 4.6 + CUDA), real rendering | needs a reboot to switch |

Recommendation: keep developing in the VM. Set up dual-boot Debian before the
end of Phase 3 (to confirm thread scaling on bare metal), and make it the
reference machine for Phase 4 and the final goal. Use WSL2 only if we want CUDA
before the dual boot exists.

None of this blocks Phases 1–3. The design work for the GPU (data layout,
order-independent resolution) happens on the CPU first anyway.

---

## Phase 1: Profiler

### 1.1 Make the measurements trustworthy

These come before any profiler code. Without them the numbers mean nothing.

- [ ] **Release builds for benchmarks.** Default `CMAKE_BUILD_TYPE` to `Release`
      when unset, and add a `RelWithDebInfo` preset for profiling with symbols.
      Record the build type in every run's metadata, and refuse to benchmark a
      Debug build (or flag it loudly).
- [ ] **Fixed timestep.** `dt = GetFrameTime()` today, so a faster build simulates
      smaller steps, which means fewer overlaps and a *different workload*. That
      makes before/after comparisons invalid. Use a fixed `dt = 1/60` for the
      simulation, independent of render rate.
- [ ] **Seeded RNG.** `SetRandomSeed(seed)` from config, with the seed logged.
      Same seed + fixed dt = the identical simulation every run.
- [ ] **Headless mode** (`--headless`): no window, no rendering, simulation steps
      only. Rendering gets measured separately; it's a different problem
      (software GL in the VM).
- [ ] **Warm-up:** discard the first N steps (caches, allocator, page faults).
- [ ] Switch `Timer` from `high_resolution_clock` to `steady_clock` (guaranteed
      monotonic).

### 1.2 Profiler design

Small, in-house, low overhead. Replaces the current `Timer` + string laps.

- **Scoped zones:** `PROFILE_ZONE(Zone::Collision);` RAII guards that record start
  and end. Zones are a fixed enum, not strings: the current `stats[name]` hashes a
  `std::string` on every lap, which is itself overhead inside the measurement.
- **Nested zones**, so `collision` can break down into `pair_gen`, `dedup`,
  `narrow_test`, `resolve`. That's where the next 9 ms actually goes.
- **Raw per-frame samples**, not running averages: a `frames × zones` table kept
  in memory and written at the end. Percentiles (p50/p95/p99) and spikes are
  computed from it afterwards. The current avg/min/max over 120 frames hides
  spikes and can't give medians.
- **Per-frame counters** alongside the times (see 1.3).
- **Compile-time switch** (`ENGINE_PROFILE=OFF` compiles zones to nothing), plus a
  measured *empty-zone cost* reported in the metadata so we know the profiler's
  own overhead.
- Later, optionally: an exporter for Tracy or Chrome's `chrome://tracing` JSON,
  for timeline views once threads exist. Not needed on day one.

### 1.3 What to record, and why it matters

**Per frame, timings:**

| metric | why it matters |
|---|---|
| `step` total, `integrate`, `walls`, `grid_build`, `collision` (+ sub-zones) | where time goes; which phase to attack |
| `render` (windowed runs only) | kept separate so it doesn't pollute physics numbers |

**Per frame, counters.** These are per-step values; today's `candidatestats` are
cumulative, which hides how things change over a run.

| metric | why it matters |
|---|---|
| candidate pairs, duplicate pairs, unique tests | broad-phase efficiency; duplicates = wasted work from bodies spanning cells |
| actual contacts | real work; contacts ÷ unique tests = how much of the broad phase was wasted |
| bodies per cell: max, mean, occupied-cell count | grid tuning; one crowded cell = O(k²) hotspot |
| heap allocations this step | allocations in the hot loop are a common hidden cost (counted by overriding `operator new` in profile builds) |

**Derived (in analysis):**

| metric | why it matters |
|---|---|
| ns per body per step, ns per unique pair test | normalised cost, comparable across body counts and machines |
| steps/s at p50 and p95 | what the goal is stated in |
| speedup and parallel efficiency vs threads (Phase 3) | shows whether threading is actually paying off |

**Per run (metadata, one JSON per run):** git commit + dirty flag, build type,
compiler + flags, scenario + all parameters, seed, dt, step count, warm-up, thread
count, CPU model, vCPU count, VM yes/no, peak RSS (`VmHWM`), profiler overhead
per zone, start timestamp.

Without the metadata, a CSV from three weeks ago can't be compared with anything.

### 1.4 Benchmark harness

- [ ] `engine_e bench <scenario> [--bodies 1000,3000,10000] [--cells ...]
      [--steps N] [--warmup N] [--repeat R] [--threads ...] [--seed S]`.
      It runs the whole sweep headless, one output folder per sweep.
- [ ] **Sweep body count *and* density.** Grid performance depends on bodies per
      cell, not just total bodies: 10k bodies on a big field and 10k packed tight
      are different workloads. Add a scenario parameter for world size or density.
- [ ] **Repeats:** default 5. Report the median of the run medians, plus spread, so
      VM noise shows up as variance instead of silently skewing results.
- [ ] Output: `frames.csv` (per-frame zones + counters), `summary.csv` (one row per
      run with percentiles), `meta.json`.

### 1.5 Analysis and comparison

- [ ] Replace or extend `anal.py` to read the new format (raw frames → percentiles).
- [ ] `compare.py baseline/ candidate/`: per-zone p50/p95 delta, flagged only if the
      change is larger than the run-to-run spread. This is the "concretely verify"
      tool: every optimisation PR gets a before/after table.
- [ ] Keep a committed `benchmarks/baseline/` summary (small CSV/JSON, not raw
      frames) as the reference point.

### 1.6 Correctness guard

Speed-ups must not quietly break physics.

- [ ] **State checksum** after N steps (hash of positions/velocities). With fixed dt
      + seed, single-thread refactors must produce *identical* checksums.
- [ ] **Invariant checks** from the existing diagnostics: momentum and energy
      drift in `mass_test` (walls off) stays within a tolerance. Needed once
      threading changes floating-point summation order and bitwise equality no
      longer holds.

**Phase 1 is done when:** one command produces a reproducible sweep; repeated runs
of the same commit agree within a few %; we can say exactly where the
collision time goes; and we've picked a concrete goal (see end).

---

## Phase 2: Single-threaded groundwork

Threading slow code multiplies waste, and the GPU needs a different data layout
anyway. Each item gets measured with Phase 1 tools, and the change is kept only
if it wins.

- [ ] **Drop the global pair hash set.** Deduplicate by rule instead: test a pair
      only in the cell that contains the top-left corner of the two AABBs' overlap.
      No shared state, O(1), and it parallelises trivially. Expected to be the
      biggest single win; verify.
- [ ] **Flat grid:** replace `vector<vector<size_t>>` with a counting sort (count
      per cell → prefix sum → scatter into one array + cell offsets). No per-cell
      allocations, cache-friendly, and the same structure works on the GPU.
- [ ] **Struct-of-arrays bodies** (positions, velocities, inv_mass, half-extents,
      shape type as separate arrays) instead of `vector<Body>` with a `variant`.
      Better cache use and SIMD; required for the GPU.
- [ ] **32-bit indices** (`uint32_t`) instead of `size_t`: halves index memory
      traffic.
- [ ] Re-tune the cell size against body size once the above lands; the optimum
      moves.

## Phase 3: Multithreading

- [ ] **Thread pool:** a small fork-join pool (`std::jthread` + `std::barrier`,
      `parallel_for(range, chunk)`). Avoid `std::execution::par`: on GCC it needs
      TBB and gives no control over threads. Thread count comes from config/CLI
      and is logged.
- [ ] **Trivially parallel first:** integrate, walls, diagnostics.
- [ ] **Grid build:** parallel counting sort with per-thread histograms.
- [ ] **Narrow-phase detection:** read-only, parallel over cells, each thread
      appending contacts to its own buffer. Merge buffers in thread-index order so
      the result is the same for any thread count.
- [ ] **Resolution:** this is the hard part, since two contacts can share a body.
      Try in this order:
      1. Resolve the merged contact list serially. Simple, and may be enough if
         contacts ≪ pair tests (measure the ratio first).
      2. **Jacobi-style:** compute each contact's impulse from the start-of-step
         state, accumulate per body, apply once. Order-independent, deterministic,
         and exactly the form the GPU needs. It may converge slower than the
         current sequential (Gauss-Seidel) approach, so check the invariants.
      3. Graph colouring / batching (contacts in a batch share no body), only if
         1 and 2 fall short.
- [ ] **Measure:** speedup vs 1..8 threads per phase, parallel efficiency, and the
      serial fraction (Amdahl). Expect flattening near 4–8 in the VM (vCPU/SMT
      mapping, host noise); confirm on bare metal.

## Phase 4: GPU (on dual-boot Debian, or WSL2 for headless CUDA; see §1)

**Prerequisites, all from Phases 2–3:** SoA layout, flat grid, rule-based
deduplication, Jacobi resolution, fixed-capacity buffers.

**API choice:**

| option | pros | cons |
|---|---|---|
| OpenGL 4.3 compute via raylib's `rlgl` (raylib built with `GRAPHICS_API_OPENGL_43`) | stays in the current stack; bodies can stay on the GPU and be drawn instanced from the same buffer, which also removes the CPU rendering cost | weaker tooling |
| CUDA | best tooling (Nsight), easiest to write | NVIDIA-only; needs CUDA↔GL interop to render without readback |
| Vulkan compute | most control | far more code |

Recommendation: OpenGL compute through raylib, unless learning CUDA is itself a goal.

**Pipeline:** integrate → compute cell key per body → radix sort by key → cell
start/end table → per-body scan of neighbouring cells → accumulate impulses →
apply. Read data back only for logging every N frames.

**Measure:** per-kernel GPU time (GL timer queries / CUDA events), transfer time,
end-to-end step time vs the best CPU multithreaded result, across body counts to
find the crossover where the GPU starts winning.

---

## Rendering track (visuals are part of the goal)

Headless runs are for physics trials; the goal itself is measured with
rendering on.

- [ ] Measure rendering separately: CPU time spent issuing draws vs time waiting
      on the GPU/present (`EndDrawing`).
- [ ] **Batch the draws.** Today it's one `DrawRectangleRec` / `DrawCircleV` call
      per body. Switch to instanced drawing: one draw call per shape type, with
      per-instance position/size/colour from a buffer.
- [ ] Turn off vsync for benchmarks (`target_fps = 0` already) and record whether
      vsync was on in the metadata.
- [ ] In Phase 4, draw straight from the GPU physics buffers, so no data goes back
      to the CPU just to draw it.
- [ ] Rendering numbers only count from the reference machine (dual-boot Debian);
      the VM renders in software.

## Accuracy budget

Speed-ups can change the simulation's results. Decide upfront how much
difference is acceptable at each level:

| level | meaning | what checks it |
|---|---|---|
| **Bitwise deterministic** | same seed → identical results, regardless of thread count | state checksum |
| **Physically valid** | momentum conserved, energy drift < X%, no tunnelling, no sinking | invariants in `mass_test` and a multi-contact scenario |
| **Visually plausible** | looks right | eyes |

Where results can change, and what each choice costs:

| source | effect | mitigation / cost |
|---|---|---|
| Resolution order (sequential → Jacobi) | multi-contact situations (chains, piles) resolve differently: momentum travels one body per step through a chain, and simultaneous contacts can overshoot (jitter, extra bounce) | relaxation factor, several iterations per step (costs time) |
| Floating-point summation order (threads, GPU atomics) | results differ in the last bits; the many-body system is chaotic, so runs diverge visibly after a few hundred steps | merge per-thread results in a fixed order (cheap on CPU); GPU float atomics are inherently non-deterministic |
| Timestep size | tunnelling once `speed × dt` exceeds body size; Euler integration drifts energy (drop test: e=1 ball already overshoots 60 → 60.6 px) | substeps (cost ×k), better integrator |
| Iterations per step (currently 1) | stacks and piles squish and jitter | more iterations (cost ×k) |
| Fixed-capacity GPU buffers (max bodies per cell / contacts per body) | overflow means collisions are silently missed | size generously, count overflows as a profiler counter |
| Positional correction (`SLOP`, `PERCENT`) | overlap allowed vs jitter | tuning |
| `-ffast-math` | reorders float maths | don't use it, or accept losing bitwise determinism |

**Approach (decided 2026-10-25):** optimise for performance first, then verify
accuracy and decide per item whether the loss is acceptable. The minimum bar is
that the simulation (mostly) never explodes. Float-order noise is acceptable.
The strictness of replicability is undecided and will be settled by trials.

Per item:

| # | item | decision |
|---|---|---|
| 1 | resolution order | measure in Phase 3: serial resolution may be enough on CPU; Jacobi or colouring for GPU |
| 2 | replicability | **active**: `replicability` scenario + `tools/compare_runs.py` (below) |
| 3 | timestep / tunnelling | profile first: build a scenario that provokes it, then decide |
| 4 | iterations per step | trial: add an iteration-count knob, measure cost vs pile quality |
| 5 | GPU buffer capacity | defer to Phase 4; count overflows |

**Replicability tooling (done):**
- `engine_e replicability`: Newton's-cradle chain + 8 circles colliding at the
  same moment, then chaotic; fixed dt, logs every frame.
- `engine_e` prints a final-state checksum after every run.
- `python3 tools/compare_runs.py <runA> <runB>` reports: bit-identical or not,
  first divergent frame, how the gap grows, and NaN / energy-gain checks per run.

First result: single-threaded with fixed dt, runs are bit-identical. Scenarios
still on frame-time dt (stress, mass_test, drop_test) diverge from frame 2. The
timestep is the only current source of non-replicability.

## Defining the goal

Pick it after Phase 1, when the baseline is real. Suggested form:

> **N bodies at ≤ X ms per frame (p95) with rendering on, fixed dt = 1/60,
> density D, on the dual-boot Debian machine, within the accuracy budget.**

For example: "100k bodies at ≤ 16.6 ms p95, rendered, on the 5900HS bare metal".
Stating density and machine matters as much as N. Headless step time gets
tracked alongside as the physics-only number.

## Open questions

- How strict replicability must be: decide after re-running the
  replicability check with threads (Phase 3).
