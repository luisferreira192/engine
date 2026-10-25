#pragma once

#include "Collider.h"

#include <raylib.h>

#include <cstddef>

// Axes along which a body is not allowed to move (e.g. a rail along x is
// AxisLock{ .y = true }).
struct AxisLock {
    bool x = false;
    bool y = false;
};

/*
         +x
(0,0) ───────→
  |
  |
  ↓ +y

  position is the body's center.

  When adding a property here, also add it to the body log in
  RunRecorder.cpp (a static_assert there trips when Body's size changes).
*/
struct Body {
    Vector2 position = {};
    Vector2 velocity = {};

    float restitution = 1.0f;   // 1 = perfectly elastic

    AxisLock locked = {};

    Collider collider;

    Body(Vector2 position, Vector2 velocity, Collider collider);

    // mass and inverse mass are kept in sync through these.
    void set_mass(float mass);
    void make_static();          // immovable: infinite mass, inv_mass = 0

    float mass() const { return mass_; }
    float inv_mass() const { return inv_mass_; }
    bool is_static() const { return inv_mass_ == 0.0f; }

    ColliderType collider_type() const;

    Vector2 half_extents() const;

    // Axis-aligned bounding box (top-left corner + size).
    Rectangle bounds() const;

private:
    float mass_ = 1.0f;
    float inv_mass_ = 1.0f;
};
