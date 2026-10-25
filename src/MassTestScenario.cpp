#include "MassTestScenario.h"

#include <cmath>

namespace {

// Visual size grows with mass so the difference is visible.
float side_for_mass(float mass)
{
    return 12.0f * std::cbrt(mass);
}

struct Lane {
    float m1, v1;   // left body
    float m2, v2;   // right body
};

constexpr Lane LANES[] = {
    {  1.0f,  100.0f,  1.0f,    0.0f },   // equal masses: velocities swap
    { 10.0f,  100.0f,  1.0f,    0.0f },   // heavy hits light: light flies off
    {  1.0f,  100.0f, 10.0f,    0.0f },   // light hits heavy: light bounces back
    {  1.0f,  100.0f,  1.0f, -100.0f },   // equal, head-on
    {  3.0f,   50.0f,  1.0f, -150.0f },   // zero total momentum
};

} // namespace

void MassTestScenario::configure(SimulationConfig& config) const
{
    config.target_fps = 60;
    config.max_frames = 60 * 10;
    config.cell_count = 64;
    config.body_log_interval = 1;
    config.output_root = "experiments/mass_test";
}

WorldSettings MassTestScenario::world_settings() const
{
    WorldSettings settings;
    settings.walls = { false, false, false, false };
    return settings;
}

void MassTestScenario::build(World& world, const SimulationConfig&)
{
    const Rectangle area = world.bounds();
    constexpr size_t lane_count = std::size(LANES);

    const float lane_height = area.height / lane_count;

    for (size_t i = 0; i < lane_count; ++i) {
        const Lane& lane = LANES[i];
        const float y = area.y + lane_height * (i + 0.5f);

        const float s1 = side_for_mass(lane.m1);
        const float s2 = side_for_mass(lane.m2);

        world.spawn(BodySpec{}
            .at(area.x + area.width * 0.3f, y)
            .moving(lane.v1, 0.0f)
            .with_mass(lane.m1)
            .box(s1, s1)
            .lock_y());

        world.spawn(BodySpec{}
            .at(area.x + area.width * 0.6f, y)
            .moving(lane.v2, 0.0f)
            .with_mass(lane.m2)
            .box(s2, s2)
            .lock_y());
    }
}
