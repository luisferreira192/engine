#include "Application.h"
#include "Config.h"
#include "Scenario.h"

#include <iostream>
#include <string_view>

// Usage: engine_e [scenario]     (default: stress)
int main(int argc, char** argv)
{
    std::string_view name = argc > 1 ? argv[1] : "stress";

    auto scenario = make_scenario(name);

    if (!scenario) {
        std::cerr << "Unknown scenario '" << name << "'. Available:";
        for (auto available : scenario_names())
            std::cerr << ' ' << available;
        std::cerr << '\n';
        return 1;
    }

    SimulationConfig config;
    scenario->configure(config);

    Application app(config, *scenario);

    return app.run();
}
