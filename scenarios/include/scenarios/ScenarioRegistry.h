#pragma once

#include "runtime/Scenario.h"

#include <memory>
#include <string_view>
#include <vector>

// Returns nullptr for an unknown name.
std::unique_ptr<Scenario> make_scenario(std::string_view name);

std::vector<std::string_view> scenario_names();
