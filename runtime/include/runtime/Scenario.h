#pragma once

#include "engine/World.h"
#include "runtime/Config.h"

// Describes the initial state of a run: environment rules and which bodies
// exist where. Concrete scenarios live in the scenarios/ layer.
class Scenario {
public:
    virtual ~Scenario() = default;

    virtual const char* name() const = 0;

    // Adjust run parameters (fps, frame count, output folder, ...).
    virtual void configure(SimulationConfig&) const {}

    virtual WorldSettings world_settings() const { return {}; }

    virtual void build(World& world, const SimulationConfig& config) = 0;
};
