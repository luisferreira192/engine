#pragma once

#include <raylib.h>

#include <cstddef>
#include <variant>

struct BoxCollider {
    float width;
    float height;
};

struct CircleCollider {
    float radius;
};

// The index of each alternative is used to look up collision dispatch
// tables, so the order here matters (see Collision.cpp).
using Collider = std::variant<BoxCollider, CircleCollider>;

enum ColliderType : size_t {
    COLLIDER_BOX    = 0,
    COLLIDER_CIRCLE = 1,
    COLLIDER_TYPE_COUNT
};

static_assert(std::variant_size_v<Collider> == COLLIDER_TYPE_COUNT);

// Half width / half height of the collider's axis-aligned bounding box.
Vector2 half_extents(const Collider& collider);
