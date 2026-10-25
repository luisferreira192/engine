#include "runtime/Renderer.h"

#include <type_traits>
#include <variant>

void Renderer::draw_world(const World& world) const
{
    for (const Body& body : world.bodies())
        draw_body(body);
}

void Renderer::draw_body(const Body& body) const
{
    const Color color = body.is_static() ? static_color : body_color;

    std::visit([&](const auto& col) {
        using T = std::decay_t<decltype(col)>;

        if constexpr (std::is_same_v<T, BoxCollider>) {
            DrawRectangleRec(body.bounds(), color);
        }
        else if constexpr (std::is_same_v<T, CircleCollider>) {
            DrawCircleV(body.position, col.radius, color);
        }
    }, body.collider);
}
