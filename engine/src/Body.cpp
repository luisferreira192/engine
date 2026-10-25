#include "engine/Body.h"

#include <limits>
#include <type_traits>
#include <utility>

Vector2 half_extents(const Collider& collider)
{
    return std::visit(
        [](const auto& col) -> Vector2 {
            using T = std::decay_t<decltype(col)>;

            if constexpr (std::is_same_v<T, BoxCollider>) {
                return { col.width / 2.0f, col.height / 2.0f };
            }
            else if constexpr (std::is_same_v<T, CircleCollider>) {
                return { col.radius, col.radius };
            }
        },
        collider
    );
}

Body::Body(Vector2 position, Vector2 velocity, Collider collider)
: position(position),
  velocity(velocity),
  collider(std::move(collider))
{
}

void Body::set_mass(float mass)
{
    if (mass <= 0.0f || mass == std::numeric_limits<float>::infinity()) {
        make_static();
        return;
    }

    mass_ = mass;
    inv_mass_ = 1.0f / mass;
}

void Body::make_static()
{
    mass_ = std::numeric_limits<float>::infinity();
    inv_mass_ = 0.0f;
    velocity = {};
}

ColliderType Body::collider_type() const
{
    return static_cast<ColliderType>(collider.index());
}

Vector2 Body::half_extents() const
{
    return ::half_extents(collider);
}

Rectangle Body::bounds() const
{
    Vector2 extents = half_extents();

    return {
        position.x - extents.x,
        position.y - extents.y,
        extents.x * 2.0f,
        extents.y * 2.0f
    };
}
