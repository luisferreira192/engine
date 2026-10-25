#pragma once

#include "runtime/Scenario.h"

#include <raylib.h>

#include <cstddef>
#include <vector>

struct StressTestSettings {
    size_t body_count = 3000;

    float min_circle_radius = 5.0f;
    float max_circle_radius = 5.0f;

    float min_box_width  = 5.0f;
    float max_box_width  = 5.0f;

    float min_box_height = 5.0f;
    float max_box_height = 5.0f;

    float initial_speed = 100.0f;
};

// Profiling setup: settings.body_count bodies spread evenly over the grid.
// Each new body goes into the currently least populated cell, laid out on a
// small lattice inside it.
class StressTestScenario : public Scenario {
public:
    explicit StressTestScenario(StressTestSettings settings = {});

    const char* name() const override { return "stress"; }

    void build(World& world, const SimulationConfig& config) override;

private:
    BodySpec make_spec(size_t i, Vector2 position) const;
    Vector2 next_position(const SpatialGrid& grid);

    StressTestSettings settings_;
    std::vector<size_t> cell_population_;
};
