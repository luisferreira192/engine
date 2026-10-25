#include "scenarios/DropTestScenario.h"

void DropTestScenario::configure(SimulationConfig& config) const
{
    config.target_fps = 60;
    config.max_frames = 60 * 15;
    config.cell_count = 64;
    config.body_log_interval = 1;
    config.output_root = "experiments/drop_test";
}

WorldSettings DropTestScenario::world_settings() const
{
    WorldSettings settings;
    settings.gravity = { 0.0f, 400.0f };
    return settings;
}

void DropTestScenario::build(World& world, const SimulationConfig&)
{
    const Rectangle area = world.bounds();

    struct Drop { float mass; float restitution; };

    constexpr Drop DROPS[] = {
        {  1.0f, 1.0f  },
        { 10.0f, 1.0f  },   // same as above, heavier: should match exactly
        {  1.0f, 0.8f  },
        {  1.0f, 0.5f  },
        {  1.0f, 0.0f  },
    };

    constexpr size_t count = std::size(DROPS);
    const float spacing = area.width / (count + 2);

    for (size_t i = 0; i < count; ++i) {
        world.spawn(BodySpec{}
            .at(area.x + spacing * (i + 1), area.y + 60.0f)
            .with_mass(DROPS[i].mass)
            .with_restitution(DROPS[i].restitution)
            .circle(8.0f)
            .lock_x());
    }

    // Static ledge with a ball falling onto it.
    const float ledge_x = area.x + spacing * (count + 1);

    world.spawn(BodySpec{}
        .at(ledge_x, area.y + area.height * 0.6f)
        .box(60.0f, 10.0f)
        .fixed());

    world.spawn(BodySpec{}
        .at(ledge_x, area.y + 60.0f)
        .with_mass(1.0f)
        .with_restitution(0.9f)
        .box(12.0f, 12.0f)
        .lock_x());
}
