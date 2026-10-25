#pragma once

#include "Config.h"
#include "World.h"

#include <memory>
#include <string_view>
#include <vector>

// Describes the initial state of a run: environment rules and which bodies
// exist where.
class Scenario {
public:
    virtual ~Scenario() = default;

    virtual const char* name() const = 0;

    // Adjust run parameters (fps, frame count, output folder, ...).
    virtual void configure(SimulationConfig&) const {}

    virtual WorldSettings world_settings() const { return {}; }

    virtual void build(World& world, const SimulationConfig& config) = 0;
};

// Returns nullptr for an unknown name.
std::unique_ptr<Scenario> make_scenario(std::string_view name);

std::vector<std::string_view> scenario_names();
