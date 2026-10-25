#pragma once

#include "engine/CollisionSystem.h"
#include "runtime/Timer.h"
#include "engine/World.h"

#include <cstddef>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

// Collects per-run measurements and writes them to
//   <root>/<YYYY-MM-DD>/<HH-MM>/<kind>_<bodies>_<cells>_<timestamp>.csv
//
//   timelog         phase timings per stats block        (written on save)
//   candidatestats  cumulative broad-phase counters      (written on save)
//   bodies          every property of every body, sampled (streamed)
//   diagnostics     momentum / kinetic energy per frame   (streamed)
//
// timelog and candidatestats keep the layout anal.py expects.
class RunRecorder {
public:
    RunRecorder(std::string root, size_t body_count, size_t cell_count);

    bool is_open() const;

    // frame = number of simulation steps taken so far (0 = initial state).
    void record_frame(size_t frame, float time, const World& world,
                      bool snapshot_bodies);

    void record_collision_stats(const CollisionStats& stats);

    bool save(const Timer& timer);

    const std::filesystem::path& directory() const { return directory_; }

private:
    static std::string timestamp();

    std::filesystem::path file_path(const std::string& prefix) const;

    size_t body_count_;
    size_t cell_count_;
    std::string date_;
    std::filesystem::path directory_;

    std::ofstream bodies_;
    std::ofstream diagnostics_;
    std::ostringstream candidate_output_;
};
