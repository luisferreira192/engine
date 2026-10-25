#include "Scenario.h"

#include "DropTestScenario.h"
#include "MassTestScenario.h"
#include "StressTestScenario.h"

std::unique_ptr<Scenario> make_scenario(std::string_view name)
{
    if (name == "stress")    return std::make_unique<StressTestScenario>();
    if (name == "mass_test") return std::make_unique<MassTestScenario>();
    if (name == "drop_test") return std::make_unique<DropTestScenario>();

    return nullptr;
}

std::vector<std::string_view> scenario_names()
{
    return { "stress", "mass_test", "drop_test" };
}
