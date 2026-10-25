#pragma once

#include "Scenario.h"

#include <raylib.h>

#include <cstddef>
#include <vector>

// Profiling setup: config.body_count bodies spread evenly over the grid.
// Each new body goes into the currently least populated cell, laid out on a
// small lattice inside it.
class StressTestScenario : public Scenario {
public:
    const char* name() const override { return "stress"; }

    void build(World& world, const SimulationConfig& config) override;

private:
    BodySpec make_spec(size_t i, Vector2 position, const SimulationConfig& config) const;
    Vector2 next_position(const SpatialGrid& grid, const SimulationConfig& config);

    std::vector<size_t> cell_population_;
};
