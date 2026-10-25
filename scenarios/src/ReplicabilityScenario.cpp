#include "scenarios/ReplicabilityScenario.h"

#include <cmath>

void ReplicabilityScenario::configure(SimulationConfig& config) const
{
    config.fixed_dt = 1.0f / 60.0f;
    config.target_fps = 60;
    config.max_frames = 60 * 20;
    config.cell_count = 64;
    config.body_log_interval = 1;
    config.output_root = "experiments/replicability";
}

void ReplicabilityScenario::build(World& world, const SimulationConfig&)
{
    const Rectangle area = world.bounds();

    // Newton's cradle: a striker hits a row of touching boxes on a rail.
    constexpr float BOX = 20.0f;
    constexpr int CHAIN = 5;
    const float rail_y = area.y + 50.0f;

    world.spawn(BodySpec{}
        .at(area.x + 150.0f, rail_y)
        .moving(150.0f, 0.0f)
        .box(BOX, BOX)
        .lock_y());

    for (int i = 0; i < CHAIN; ++i) {
        world.spawn(BodySpec{}
            .at(area.x + 400.0f + i * BOX, rail_y)
            .box(BOX, BOX)
            .lock_y());
    }

    // Ring of circles converging on the centre at the same speed.
    constexpr int RING = 8;
    constexpr float RING_RADIUS = 120.0f;
    constexpr float SPEED = 80.0f;
    const Vector2 centre = { area.x + area.width / 2.0f, area.y + 280.0f };

    for (int i = 0; i < RING; ++i) {
        const float angle = (2.0f * PI * i) / RING;
        const Vector2 dir = { std::cos(angle), std::sin(angle) };
        const float mass = 1.0f + static_cast<float>(i % 4);

        world.spawn(BodySpec{}
            .at(centre.x + dir.x * RING_RADIUS, centre.y + dir.y * RING_RADIUS)
            .moving(-dir.x * SPEED, -dir.y * SPEED)
            .with_mass(mass)
            .circle(8.0f + 2.0f * static_cast<float>(i % 4)));
    }
}
