#include "World.h"

#include <utility>

World::World(Rectangle bounds, size_t cell_count, WorldSettings settings)
: bounds_(bounds),
  settings_(settings),
  grid_(bounds, cell_count)
{
}

void World::add_body(Body body)
{
    bodies_.push_back(std::move(body));
}

void World::spawn(const BodySpec& spec)
{
    add_body(spec.build(settings_.default_restitution));
}

void World::prepare_simulation()
{
    collisions_.reserve(bodies_.size());
    update_grid();
}

void World::integrate(float dt)
{
    const Vector2 gravity = settings_.gravity;

    for (Body& body : bodies_) {
        if (body.is_static())
            continue;

        if (!body.locked.x) {
            body.velocity.x += gravity.x * dt;
            body.position.x += body.velocity.x * dt;
        }

        if (!body.locked.y) {
            body.velocity.y += gravity.y * dt;
            body.position.y += body.velocity.y * dt;
        }
    }
}

void World::resolve_wall_collisions()
{
    const WorldSettings::Walls& walls = settings_.walls;

    const float left_wall   = bounds_.x;
    const float right_wall  = bounds_.x + bounds_.width;
    const float top_wall    = bounds_.y;
    const float bottom_wall = bounds_.y + bounds_.height;

    for (Body& body : bodies_) {
        if (body.is_static())
            continue;

        Vector2 extents = body.half_extents();
        const float bounce = -body.restitution;

        if (walls.left && body.position.x - extents.x < left_wall) {
            body.position.x = left_wall + extents.x;

            if (body.velocity.x < 0.0f)
                body.velocity.x *= bounce;
        }

        if (walls.right && body.position.x + extents.x > right_wall) {
            body.position.x = right_wall - extents.x;

            if (body.velocity.x > 0.0f)
                body.velocity.x *= bounce;
        }

        if (walls.top && body.position.y - extents.y < top_wall) {
            body.position.y = top_wall + extents.y;

            if (body.velocity.y < 0.0f)
                body.velocity.y *= bounce;
        }

        if (walls.bottom && body.position.y + extents.y > bottom_wall) {
            body.position.y = bottom_wall - extents.y;

            if (body.velocity.y > 0.0f)
                body.velocity.y *= bounce;
        }
    }
}

void World::update_grid()
{
    grid_.rebuild(bodies_);
}

void World::resolve_body_collisions()
{
    collisions_.resolve(bodies_, grid_);
}

WorldDiagnostics World::diagnostics() const
{
    WorldDiagnostics d;

    for (const Body& body : bodies_) {
        if (body.is_static())
            continue;

        const float m = body.mass();
        const Vector2 v = body.velocity;

        d.momentum.x += m * v.x;
        d.momentum.y += m * v.y;
        d.kinetic_energy += 0.5f * m * (v.x * v.x + v.y * v.y);
        ++d.dynamic_bodies;
    }

    return d;
}
