#include "scenarios/ScenarioRegistry.h"

#include "scenarios/DropTestScenario.h"
#include "scenarios/MassTestScenario.h"
#include "scenarios/ReplicabilityScenario.h"
#include "scenarios/StressTestScenario.h"

namespace {

struct Entry {
    std::string_view name;
    std::unique_ptr<Scenario> (*create)();
};

template <typename T>
std::unique_ptr<Scenario> create()
{
    return std::make_unique<T>();
}

// Register new scenarios here.
constexpr Entry REGISTRY[] = {
    { "stress",        create<StressTestScenario>    },
    { "mass_test",     create<MassTestScenario>      },
    { "drop_test",     create<DropTestScenario>      },
    { "replicability", create<ReplicabilityScenario> },
};

} // namespace

std::unique_ptr<Scenario> make_scenario(std::string_view name)
{
    for (const Entry& entry : REGISTRY) {
        if (entry.name == name)
            return entry.create();
    }

    return nullptr;
}

std::vector<std::string_view> scenario_names()
{
    std::vector<std::string_view> names;

    for (const Entry& entry : REGISTRY)
        names.push_back(entry.name);

    return names;
}
