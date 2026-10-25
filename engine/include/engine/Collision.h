#pragma once

#include "engine/Body.h"

#include <raylib.h>

// Narrow phase: shape-vs-shape tests and contact resolution.
namespace collision {

struct Contact {
    Vector2 normal;   // unit vector pointing from a to b
    float depth;      // penetration along the normal
};

bool intersects(const Body& a, const Body& b);

Contact contact(const Body& a, const Body& b);

// Separate overlapping bodies and, if they are approaching, apply an impulse
// along the contact normal. Respects static bodies and axis locks;
// restitution is the lower of the two.
void resolve(Body& a, Body& b, const Contact& contact);

} // namespace collision
