#pragma once

#include "engine/Body.h"
#include "engine/World.h"

#include <raylib.h>

class Renderer {
public:
    void draw_world(const World& world) const;
    void draw_body(const Body& body) const;

    Color body_color = RED;
    Color static_color = DARKGRAY;
    Color background = RAYWHITE;
};
