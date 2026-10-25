#include <raylib.h>

/*
         +x
(0,0) ───────→
  |
  |
  ↓ +y
  
*/

Rectangle centered_rectangle(Vector2 center, float width, float height)
{
    return {
        center.x - width / 2.0f,
        center.y - height / 2.0f,
        width,
        height
    };
}

bool check_wall_colision(Rectangle rec, double axis_speed, bool axis)
{
    if (!axis && ((rec.x >= GetScreenWidth() - rec.width && axis_speed > 0.0f) || (rec.x <= 0.0 && axis_speed < 0.0f))) return true;
    if (axis && ((rec.y >= GetScreenHeight() - rec.height && axis_speed > 0.0f) || (rec.y <= 0.0 && axis_speed < 0.0f))) return true;

    return false;
}

int main()
{
    constexpr int width = 800;
    constexpr int height = 450;

    InitWindow(width, height, "raylib test");

    SetTargetFPS(60);

    Vector2 center = { GetScreenWidth()/2.0f, GetScreenHeight()/2.0f };

    constexpr int text_size = 30;
    constexpr float padding = 10.0f;

    const char* text = "raylib works!";

    float text_width = MeasureText(text, text_size);
    float text_height = text_size;

    Rectangle box{
        center.x - text_width / 2.0f - padding / 2.0f,
        center.y - text_height / 2.0f - padding / 2.0f,
        text_width + padding,
        text_height + padding
    };

    float speed_x = 300.0f; // pixels per second
    float speed_y = 300.0f;

    while (!WindowShouldClose())
    {
        BeginDrawing();

        ClearBackground(RAYWHITE);

        if (check_wall_colision(box, speed_x, false)) speed_x *= -1.0f;
        if (check_wall_colision(box, speed_y, true)) speed_y *= -1.0f;

        float dt = GetFrameTime();
        box.x += speed_x * dt;
        box.y += speed_y * dt;

        DrawRectangleRec(box, GREEN);

        DrawText(
            text,
            box.x + padding/2.0f,
            box.y + padding/2.0f,
            text_size,
            BLACK
        );

        EndDrawing();
    }

    CloseWindow();

    return 0;
}