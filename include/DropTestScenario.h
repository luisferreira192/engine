#pragma once

#include "Scenario.h"

// Gravity + restitution: bodies with different masses and restitutions
// dropped from the same height onto the floor, plus a static ledge.
// Mass should not change fall speed; restitution should change bounce height.
class DropTestScenario : public Scenario {
public:
    const char* name() const override { return "drop_test"; }

    void configure(SimulationConfig& config) const override;
    WorldSettings world_settings() const override;
    void build(World& world, const SimulationConfig& config) override;
};
