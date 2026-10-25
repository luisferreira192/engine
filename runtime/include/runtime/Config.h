#pragma once

#include <cstddef>
#include <string>

// Run parameters shared by every scenario. Scenarios may override any of
// these in Scenario::configure(); scenario-specific knobs live with the
// scenario itself.
struct SimulationConfig {
    // Window
    int screen_width  = 800;
    int screen_height = 450;
    int target_fps    = 0;      // 0 = uncapped (profiling)

    // World
    size_t cell_count = 400;    // must be a perfect square (rows x rows grid)

    // Profiling / logging
    size_t stats_interval    = 120;       // frames per statistics block
    size_t body_log_interval = 120;       // frames between body snapshots, 0 = off
    size_t max_frames        = 120 * 12;
    std::string output_root  = "timings";
};
