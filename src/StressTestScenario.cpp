#include "StressTestScenario.h"

#include <algorithm>
#include <cmath>
#include <iterator>

namespace {

constexpr size_t BODIES_PER_ROW = 4;    // lattice width inside a cell
constexpr float  CELL_PADDING   = 10.0f;
constexpr float  BODY_SPACING   = 5.0f;

float lerp(float min, float max, float t)
{
    return min + t * (max - min);
}

Vector2 random_velocity(float speed)
{
    float angle = GetRandomValue(0, 359) * DEG2RAD;

    return { std::cos(angle) * speed, std::sin(angle) * speed };
}

} // namespace

void StressTestScenario::build(World& world, const SimulationConfig& config)
{
    cell_population_.assign(world.grid().cell_count(), 0);

    for (size_t i = 0; i < config.body_count; ++i)
        world.spawn(make_spec(i, next_position(world.grid(), config), config));
}

BodySpec StressTestScenario::make_spec(size_t i, Vector2 position,
                                       const SimulationConfig& c) const
{
    Vector2 velocity = random_velocity(c.initial_speed);

    BodySpec spec;
    spec.at(position.x, position.y).moving(velocity.x, velocity.y);

    // Alternate circles and boxes, cycling through the size ranges.
    if (i % 2 == 0) {
        float t = static_cast<float>(i % 5) / 4.0f;
        float radius = lerp(c.min_circle_radius, c.max_circle_radius, t);

        return spec.circle(radius).with_mass(radius);
    }

    float tx = static_cast<float>(i % 4) / 3.0f;
    float ty = static_cast<float>((i + 2) % 4) / 3.0f;

    float width  = lerp(c.min_box_width,  c.max_box_width,  tx);
    float height = lerp(c.min_box_height, c.max_box_height, ty);

    return spec.box(width, height).with_mass(width);
}

Vector2 StressTestScenario::next_position(const SpatialGrid& grid,
                                          const SimulationConfig& config)
{
    auto least_populated =
        std::min_element(cell_population_.begin(), cell_population_.end());

    size_t cell_index =
        static_cast<size_t>(std::distance(cell_population_.begin(), least_populated));

    // Number already assigned to this cell.
    size_t local_index = (*least_populated)++;

    size_t local_x = local_index % BODIES_PER_ROW;
    size_t local_y = local_index / BODIES_PER_ROW;

    // Lattice pitch is sized for the largest body so neighbours never overlap.
    const float max_width =
        std::max(config.max_box_width, config.max_circle_radius * 2.0f);
    const float max_height =
        std::max(config.max_box_height, config.max_circle_radius * 2.0f);

    const Rectangle cell = grid.cell_rect(cell_index);

    return {
        cell.x + CELL_PADDING + max_width / 2.0f
            + local_x * (max_width + BODY_SPACING),

        cell.y + CELL_PADDING + max_height / 2.0f
            + local_y * (max_height + BODY_SPACING)
    };
}
