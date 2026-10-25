#pragma once

#include "Body.h"

#include <raylib.h>

#include <optional>

// Fluent description of a body to spawn:
//
//   BodySpec{}.at(200, 100).moving(100, 0).with_mass(10).box(20, 20).lock_y()
//
// Anything left unset falls back to a default (restitution comes from the
// world's settings).
class BodySpec {
public:
    BodySpec& at(float x, float y)            { position_ = {x, y}; return *this; }
    BodySpec& moving(float vx, float vy)      { velocity_ = {vx, vy}; return *this; }
    BodySpec& with_mass(float mass)           { mass_ = mass; return *this; }
    BodySpec& with_restitution(float e)       { restitution_ = e; return *this; }

    BodySpec& box(float width, float height)  { collider_ = BoxCollider{width, height}; return *this; }
    BodySpec& circle(float radius)            { collider_ = CircleCollider{radius}; return *this; }

    BodySpec& lock_x()                        { locked_.x = true; return *this; }
    BodySpec& lock_y()                        { locked_.y = true; return *this; }
    BodySpec& fixed()                         { fixed_ = true; return *this; }

    Body build(float default_restitution) const;

private:
    Vector2 position_ = {};
    Vector2 velocity_ = {};
    float mass_ = 1.0f;
    std::optional<float> restitution_;
    Collider collider_ = BoxCollider{10.0f, 10.0f};
    AxisLock locked_ = {};
    bool fixed_ = false;
};
