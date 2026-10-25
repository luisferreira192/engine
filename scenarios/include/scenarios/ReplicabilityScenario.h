#pragma once

#include "runtime/Scenario.h"

// Small, fully deterministic setup for checking that two runs produce the
// same result. Contains the cases most sensitive to evaluation order:
//
//   - a Newton's-cradle chain of touching boxes on a rail (multi-contact)
//   - a ring of circles of different masses converging on one point
//     (simultaneous contacts), which then scatter chaotically off the walls,
//     so any tiny difference between runs grows until it's visible
//
// Uses a fixed timestep. Compare two runs with tools/compare_runs.py.
class ReplicabilityScenario : public Scenario {
public:
    const char* name() const override { return "replicability"; }

    void configure(SimulationConfig& config) const override;
    void build(World& world, const SimulationConfig& config) override;
};
