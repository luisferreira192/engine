#pragma once

#include <cstddef>
#include <string>

// All tunable parameters for a simulation run, in one place. Scenarios may
// override any of these in Scenario::configure().
struct SimulationConfig {
    // Window
    int screen_width  = 800;
    int screen_height = 450;
    int target_fps    = 0;      // 0 = uncapped (profiling)

    // World
    size_t body_count = 3000;   // used by scenarios that generate bodies
    size_t cell_count = 400;    // must be a perfect square (rows x rows grid)

    // Bodies (stress test)
    float min_circle_radius = 5.0f;
    float max_circle_radius = 5.0f;

    float min_box_width  = 5.0f;
    float max_box_width  = 5.0f;

    float min_box_height = 5.0f;
    float max_box_height = 5.0f;

    float initial_speed = 100.0f;

    // Profiling / logging
    size_t stats_interval    = 120;       // frames per statistics block
    size_t body_log_interval = 120;       // frames between body snapshots, 0 = off
    size_t max_frames        = 120 * 12;
    std::string output_root  = "timings";
};
