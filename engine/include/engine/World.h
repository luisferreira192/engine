#pragma once

#include "engine/Body.h"
#include "engine/BodySpec.h"
#include "engine/CollisionSystem.h"
#include "engine/SpatialGrid.h"

#include <raylib.h>

#include <cstddef>
#include <vector>

// Environment rules, set per scenario.
struct WorldSettings {
    Vector2 gravity = {0.0f, 0.0f};       // px/s^2, +y is down

    struct Walls {
        bool left   = true;
        bool right  = true;
        bool top    = true;
        bool bottom = true;
    } walls;

    float default_restitution = 1.0f;     // for specs that don't set one
};

// Conserved quantities over the dynamic (non-static) bodies.
struct WorldDiagnostics {
    Vector2 momentum = {};
    float kinetic_energy = 0.0f;
    size_t dynamic_bodies = 0;
};

// Owns the bodies and runs the simulation phases. Each phase is exposed
// separately so the caller can time them individually.
class World {
public:
    World(Rectangle bounds, size_t cell_count, WorldSettings settings = {});

    void add_body(Body body);
    void spawn(const BodySpec& spec);

    // Call once after all bodies are spawned.
    void prepare_simulation();

    void integrate(float dt);
    void resolve_wall_collisions();
    void update_grid();
    void resolve_body_collisions();

    WorldDiagnostics diagnostics() const;

    Rectangle bounds() const { return bounds_; }
    const WorldSettings& settings() const { return settings_; }
    const std::vector<Body>& bodies() const { return bodies_; }
    const SpatialGrid& grid() const { return grid_; }
    const CollisionStats& collision_stats() const { return collisions_.stats(); }

private:
    Rectangle bounds_;
    WorldSettings settings_;
    std::vector<Body> bodies_;
    SpatialGrid grid_;
    CollisionSystem collisions_;
};
