#pragma once

#include "Scenario.h"

// Head-on collisions on horizontal rails (y locked), one experiment per lane,
// so the outcome can be checked against the 1D elastic collision formulas:
//
//   v1' = (m1 - m2) / (m1 + m2) * v1 + 2 m2 / (m1 + m2) * v2
//   v2' = 2 m1 / (m1 + m2) * v1 + (m2 - m1) / (m1 + m2) * v2
//
// Walls are off so momentum and energy should stay constant all run.
class MassTestScenario : public Scenario {
public:
    const char* name() const override { return "mass_test"; }

    void configure(SimulationConfig& config) const override;
    WorldSettings world_settings() const override;
    void build(World& world, const SimulationConfig& config) override;
};
