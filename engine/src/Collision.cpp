#include "engine/Collision.h"

#include <algorithm>
#include <cmath>
#include <variant>

namespace {

Rectangle box_rect(const Body& body, const BoxCollider& box)
{
    return {
        body.position.x - box.width / 2.0f,
        body.position.y - box.height / 2.0f,
        box.width,
        box.height
    };
}

// ---------------------------------------------------------
// Intersection tests
// ---------------------------------------------------------

bool box_box(const Body& a, const Body& b)
{
    const auto& box_a = std::get<BoxCollider>(a.collider);
    const auto& box_b = std::get<BoxCollider>(b.collider);

    return CheckCollisionRecs(box_rect(a, box_a), box_rect(b, box_b));
}

bool circle_circle(const Body& a, const Body& b)
{
    const auto& circ_a = std::get<CircleCollider>(a.collider);
    const auto& circ_b = std::get<CircleCollider>(b.collider);

    return CheckCollisionCircles(a.position, circ_a.radius, b.position, circ_b.radius);
}

bool box_circle(const Body& box_body, const Body& circle_body)
{
    const auto& box  = std::get<BoxCollider>(box_body.collider);
    const auto& circ = std::get<CircleCollider>(circle_body.collider);

    return CheckCollisionCircleRec(circle_body.position, circ.radius, box_rect(box_body, box));
}

bool circle_box(const Body& a, const Body& b)
{
    return box_circle(b, a);
}

// ---------------------------------------------------------
// Contacts: normal (a -> b) and penetration depth
// ---------------------------------------------------------

using collision::Contact;

Contact box_box_contact(const Body& a, const Body& b)
{
    const auto& box_a = std::get<BoxCollider>(a.collider);
    const auto& box_b = std::get<BoxCollider>(b.collider);

    float dx = b.position.x - a.position.x;
    float dy = b.position.y - a.position.y;

    float overlap_x = (box_a.width  + box_b.width)  / 2.0f - std::abs(dx);
    float overlap_y = (box_a.height + box_b.height) / 2.0f - std::abs(dy);

    if (overlap_x < overlap_y)
        return { { dx >= 0.0f ? 1.0f : -1.0f, 0.0f }, overlap_x };

    return { { 0.0f, dy >= 0.0f ? 1.0f : -1.0f }, overlap_y };
}

Contact circle_circle_contact(const Body& a, const Body& b)
{
    const float radii =
        std::get<CircleCollider>(a.collider).radius +
        std::get<CircleCollider>(b.collider).radius;

    float dx = b.position.x - a.position.x;
    float dy = b.position.y - a.position.y;

    float dist_sq = dx * dx + dy * dy;

    if (dist_sq < 0.0001f)
        return { { 1.0f, 0.0f }, radii };

    float dist = std::sqrt(dist_sq);

    return { { dx / dist, dy / dist }, radii - dist };
}

Contact box_circle_contact(const Body& box_body, const Body& circle_body)
{
    const Rectangle rect =
        box_rect(box_body, std::get<BoxCollider>(box_body.collider));

    const float radius = std::get<CircleCollider>(circle_body.collider).radius;
    const Vector2 c = circle_body.position;

    float closest_x = std::clamp(c.x, rect.x, rect.x + rect.width);
    float closest_y = std::clamp(c.y, rect.y, rect.y + rect.height);

    float dx = c.x - closest_x;
    float dy = c.y - closest_y;

    float dist_sq = dx * dx + dy * dy;

    if (dist_sq > 0.0001f) {
        float dist = std::sqrt(dist_sq);
        return { { dx / dist, dy / dist }, radius - dist };
    }

    // Circle center is inside the box: push out through the nearest face.
    float left   = c.x - rect.x;
    float right  = rect.x + rect.width - c.x;
    float top    = c.y - rect.y;
    float bottom = rect.y + rect.height - c.y;

    float minimum = std::min({ left, right, top, bottom });
    float depth = minimum + radius;

    if (minimum == left)  return { { -1.0f, 0.0f }, depth };
    if (minimum == right) return { {  1.0f, 0.0f }, depth };
    if (minimum == top)   return { { 0.0f, -1.0f }, depth };

    return { { 0.0f, 1.0f }, depth };
}

Contact circle_box_contact(const Body& circle_body, const Body& box_body)
{
    Contact c = box_circle_contact(box_body, circle_body);
    c.normal = { -c.normal.x, -c.normal.y };
    return c;
}

// Inverse mass per axis: a locked axis behaves as infinitely heavy.
Vector2 axis_inv_mass(const Body& body)
{
    return {
        body.locked.x ? 0.0f : body.inv_mass(),
        body.locked.y ? 0.0f : body.inv_mass()
    };
}

// ---------------------------------------------------------
// Dispatch tables, indexed by [ColliderType a][ColliderType b]
// ---------------------------------------------------------

using IntersectFn = bool(*)(const Body&, const Body&);
using ContactFn   = Contact(*)(const Body&, const Body&);

constexpr IntersectFn intersect_table[COLLIDER_TYPE_COUNT][COLLIDER_TYPE_COUNT] = {
    { box_box,    box_circle    },
    { circle_box, circle_circle }
};

constexpr ContactFn contact_table[COLLIDER_TYPE_COUNT][COLLIDER_TYPE_COUNT] = {
    { box_box_contact,    box_circle_contact    },
    { circle_box_contact, circle_circle_contact }
};

} // namespace

namespace collision {

bool intersects(const Body& a, const Body& b)
{
    return intersect_table[a.collider_type()][b.collider_type()](a, b);
}

Contact contact(const Body& a, const Body& b)
{
    return contact_table[a.collider_type()][b.collider_type()](a, b);
}

void resolve(Body& a, Body& b, const Contact& contact)
{
    const Vector2 n = contact.normal;

    const Vector2 inv_a = axis_inv_mass(a);
    const Vector2 inv_b = axis_inv_mass(b);

    // Effective inverse mass along the normal.
    const float inv_mass_sum =
        (inv_a.x + inv_b.x) * n.x * n.x +
        (inv_a.y + inv_b.y) * n.y * n.y;

    // Neither body can move along the normal.
    if (inv_mass_sum <= 0.0f)
        return;

    // Positional correction: push the bodies apart so they don't sink into
    // each other (needed for resting contact, e.g. under gravity).
    constexpr float SLOP = 0.01f;      // allowed overlap, avoids jitter
    constexpr float PERCENT = 0.8f;    // fraction of the overlap fixed per step

    const float correction =
        std::max(contact.depth - SLOP, 0.0f) * PERCENT / inv_mass_sum;

    a.position.x -= correction * inv_a.x * n.x;
    a.position.y -= correction * inv_a.y * n.y;

    b.position.x += correction * inv_b.x * n.x;
    b.position.y += correction * inv_b.y * n.y;

    // Velocity impulse, only if the bodies are approaching.
    float rvx = b.velocity.x - a.velocity.x;
    float rvy = b.velocity.y - a.velocity.y;

    float velocity_along_normal = rvx * n.x + rvy * n.y;

    if (velocity_along_normal >= 0.0f)
        return;

    const float restitution = std::min(a.restitution, b.restitution);

    float impulse =
        -(1.0f + restitution) * velocity_along_normal / inv_mass_sum;

    a.velocity.x -= impulse * inv_a.x * n.x;
    a.velocity.y -= impulse * inv_a.y * n.y;

    b.velocity.x += impulse * inv_b.x * n.x;
    b.velocity.y += impulse * inv_b.y * n.y;
}

} // namespace collision
