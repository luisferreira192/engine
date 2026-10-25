"""
Compare two runs of the same scenario for replicability.

    python3 tools/compare_runs.py <run_dir_a> <run_dir_b>

A run dir is the folder RunRecorder writes (contains bodies_*.csv and
diagnostics_*.csv). Floats are logged with max_digits10, so equal text means
equal bits.

Reports:
  - whether the two runs are bit-identical
  - the first frame/body/property where they differ
  - how the difference grows over time (max position gap per milestone)
  - a sanity check of each run on its own: NaN/inf, kinetic energy growth
"""

import sys
from pathlib import Path

import numpy as np
import pandas as pd


STATE = ["pos_x", "pos_y", "vel_x", "vel_y"]


def load(run_dir: Path, prefix: str) -> pd.DataFrame:
    files = sorted(run_dir.glob(f"{prefix}_*.csv"))
    if not files:
        sys.exit(f"no {prefix}_*.csv in {run_dir}")
    return pd.read_csv(files[0])


def sanity(name: str, bodies: pd.DataFrame, diag: pd.DataFrame) -> None:
    finite = np.isfinite(bodies[STATE].to_numpy()).all()

    # Total energy should never rise (it can fall: restitution < 1 loses
    # energy). Potential energy has an arbitrary zero, so measure the rise
    # relative to the peak kinetic energy instead of as a ratio.
    energy = diag["total_energy"].to_numpy()
    scale = max(diag["kinetic_energy"].max(), 1e-9)
    gain = (energy.max() - energy[0]) / scale

    verdict = "OK"
    if not finite:
        verdict = "EXPLODED (NaN/inf)"
    elif gain > 0.05:
        verdict = "ENERGY GAIN above 5%"

    print(f"  {name}: frames={bodies.frame.max()}  finite={finite}  "
          f"energy gain={gain:+.2%} of peak KE  -> {verdict}")


def main() -> None:
    if len(sys.argv) != 3:
        sys.exit(__doc__)

    dir_a, dir_b = Path(sys.argv[1]), Path(sys.argv[2])
    a, b = load(dir_a, "bodies"), load(dir_b, "bodies")

    print("Sanity")
    sanity("A", a, load(dir_a, "diagnostics"))
    sanity("B", b, load(dir_b, "diagnostics"))

    merged = a.merge(b, on=["frame", "id"], suffixes=("_a", "_b"))
    if len(merged) != len(a) or len(merged) != len(b):
        print(f"\nWarning: runs cover different frames/bodies "
              f"({len(a)} vs {len(b)} rows); comparing the overlap.")

    diff = pd.DataFrame({c: merged[f"{c}_a"] - merged[f"{c}_b"] for c in STATE})
    diff["frame"], diff["id"] = merged["frame"], merged["id"]

    differs = (diff[STATE] != 0).any(axis=1)

    print("\nReplicability")
    if not differs.any():
        print(f"  IDENTICAL: all {len(merged)} body states match bit for bit.")
        return

    first = diff[differs].iloc[0]
    column = next(c for c in STATE if first[c] != 0)
    print(f"  DIVERGED at frame {int(first.frame)} "
          f"(body {int(first.id)}, {column} differs by {first[column]:.3g})")

    gap = diff.assign(pos_gap=np.hypot(diff.pos_x, diff.pos_y))
    per_frame = gap.groupby("frame").pos_gap.max()

    print("\n  max position gap between runs (px):")
    last = int(per_frame.index.max())
    milestones = sorted({int(first.frame), *range(0, last + 1, max(last // 10, 1)), last})
    for f in milestones:
        if f in per_frame.index:
            print(f"    frame {f:5d}: {per_frame[f]:.6g}")


if __name__ == "__main__":
    main()
