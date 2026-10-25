#include <cmath>
#include <cstddef>
#include <raylib.h>
#include <vector>
#include <math.h>
#include <iostream>
#include <unordered_set>
#include <utility>
		

constexpr int WIDTH = 800;
constexpr int HEIGHT = 450;

constexpr int NUM_OF_OBJECTS = 64;
constexpr int OBJECTS_WIDTH = 10;
constexpr int OBJECTS_HEIGHT = 10;

constexpr int NUM_CELLS = 9;
const int ROWS = sqrt(NUM_CELLS);
const float CELL_WIDTH = WIDTH*(1.0f/ROWS);
const float CELL_HEIGHT = HEIGHT*(1.0f/ROWS);

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

void resolve_wall_collision(Rectangle rec, Vector2& obj_speed)
{
    if ((rec.x >= GetScreenWidth() - rec.width && obj_speed.x > 0.0f) || (rec.x <= 0.0 && obj_speed.x < 0.0f)) obj_speed.x *= -1.0f;
    if ((rec.y >= GetScreenHeight() - rec.height && obj_speed.y > 0.0f) || (rec.y <= 0.0 && obj_speed.y < 0.0f)) obj_speed.y *= -1.0f;
}

size_t get_least_populated_cell(const std::vector<std::vector<size_t>>& cells)
{
    if (cells.empty())
        return 0;

    size_t ret = 0;
    size_t smallest_cell_size = cells[0].size();

    for (size_t i = 1; i < cells.size(); ++i) {
        if (cells[i].size() < smallest_cell_size) {
            smallest_cell_size = cells[i].size();
            ret = i;
        }
    }

    return ret;
}

void place_obj_in_cell(
    std::vector<Rectangle>& objects,
    std::vector<std::vector<size_t>>& cells)
{
    size_t cell_n = get_least_populated_cell(cells);

    size_t cell_x = cell_n / ROWS;
    size_t cell_y = cell_n % ROWS;

    size_t local_idx = cells[cell_n].size();

    constexpr size_t PER_ROW = 4;

    size_t local_x = local_idx % PER_ROW;
    size_t local_y = local_idx / PER_ROW;

    float spacing_x = OBJECTS_WIDTH + 5.0f;
    float spacing_y = OBJECTS_HEIGHT + 5.0f;

    float x =
        CELL_WIDTH * cell_x +
        10.0f +
        local_x * spacing_x;

    float y =
        CELL_HEIGHT * cell_y +
        10.0f +
        local_y * spacing_y;

    Rectangle box{
        x,
        y,
        OBJECTS_WIDTH,
        OBJECTS_HEIGHT
    };

    objects.push_back(box);
    cells[cell_n].push_back(objects.size() - 1);
}

void init_objects(
    std::vector<Rectangle>& objects,
    std::vector<Vector2>& objects_speed,
    std::vector<std::vector<size_t>>& cells)
{
    cells.resize(NUM_CELLS);

    objects.reserve(NUM_OF_OBJECTS);
    objects_speed.reserve(NUM_OF_OBJECTS);

    for (int i = 0; i < NUM_OF_OBJECTS; ++i) {
        place_obj_in_cell(objects, cells);
        
        float angle = GetRandomValue(0, 359) * DEG2RAD;
        float speed = 100.0f;

        objects_speed.push_back({
            std::cos(angle) * speed,
            std::sin(angle) * speed
        });
    }
}

struct PairHash {
    size_t operator()(const std::pair<size_t, size_t>& p) const noexcept
    {
        return std::hash<size_t>{}(p.first)
             ^ (std::hash<size_t>{}(p.second) << 1);
    }
};

void check_obj_collision(
    const std::vector<Rectangle>& objects,
    const std::vector<std::vector<size_t>>& cells,
    std::vector<Vector2>& objects_speed,
    std::unordered_set<std::pair<size_t, size_t>, PairHash>& collision_state)
{
    collision_state.clear();
    
    for (const auto& cell : cells) {
        for (size_t i = 0; i < cell.size(); ++i) {
            for (size_t j = i + 1; j < cell.size(); ++j) {

                size_t a_idx = cell[i];
                size_t b_idx = cell[j];
                
                auto pair = std::minmax(a_idx, b_idx);

                if (!collision_state.insert(pair).second)
                    continue;

                const Rectangle& a = objects[a_idx];
                const Rectangle& b = objects[b_idx];

                if (!CheckCollisionRecs(a, b))
                    continue;

                float overlap_x =
                    std::min(a.x + a.width, b.x + b.width)
                    - std::max(a.x, b.x);

                float overlap_y =
                    std::min(a.y + a.height, b.y + b.height)
                    - std::max(a.y, b.y);

                if (overlap_x < overlap_y) {
                    // Hit from left/right
                    float relative_vx = objects_speed[a_idx].x - objects_speed[b_idx].x;

                    bool approaching =
                        (a.x < b.x && relative_vx > 0.0f) ||
                        (a.x > b.x && relative_vx < 0.0f);

                    if (approaching)
                        std::swap(objects_speed[a_idx].x, objects_speed[b_idx].x);
                }
                else {
                    // Hit from above/below
                    float relative_vy = objects_speed[a_idx].y - objects_speed[b_idx].y;

                    bool approaching =
                        (a.y < b.y && relative_vy > 0.0f) ||
                        (a.y > b.y && relative_vy < 0.0f);

                    if (approaching)
                        std::swap(objects_speed[a_idx].y, objects_speed[b_idx].y);
                }
            }
        }
    }
}

void move_object(Rectangle& obj, Vector2& obj_speed, float dt)
{
    obj.x += obj_speed.x * dt;
    obj.y += obj_speed.y * dt;
}

void rebuild_cells(
    const std::vector<Rectangle>& objects,
    std::vector<std::vector<size_t>>& cells)
{
    for (auto& cell : cells)
        cell.clear();

    for (size_t i = 0; i < objects.size(); ++i) {
        const Rectangle& obj = objects[i];

        float x = std::max(0.0f, obj.x);
        float y = std::max(0.0f, obj.y);

        size_t min_cell_x  = x / CELL_WIDTH;
        size_t max_cell_x  = (x + obj.width) / CELL_WIDTH;

        size_t min_cell_y = y / CELL_HEIGHT;
        size_t max_cell_y = (y + obj.height) / CELL_HEIGHT;

        if (max_cell_x >= ROWS) max_cell_x = ROWS - 1;
        if (max_cell_y >= ROWS) max_cell_y = ROWS - 1;

        for (size_t cell_x = min_cell_x; cell_x <= max_cell_x; ++cell_x) {
            for (size_t cell_y = min_cell_y; cell_y <= max_cell_y; ++cell_y) {
                size_t cell_n = cell_x * ROWS + cell_y;
                cells[cell_n].push_back(i);
            }
        }
    }
}

int main()
{
    InitWindow(WIDTH, HEIGHT, "raylib test");

    SetTargetFPS(60);

    std::vector<Rectangle> objects;
    std::vector<Vector2> objects_speed;
    std::vector<std::vector<size_t>> cells;
    init_objects(objects, objects_speed, cells);

    std::unordered_set<std::pair<size_t, size_t>, PairHash> collision_state;
    collision_state.reserve( ( NUM_OF_OBJECTS * (NUM_OF_OBJECTS - 1) ) / 2 );

    while (!WindowShouldClose())
    {
        BeginDrawing();

        ClearBackground(RAYWHITE);

        float dt = GetFrameTime();

        for (size_t i = 0; i < objects.size(); ++i)
            move_object(objects[i], objects_speed[i], dt);

        rebuild_cells(objects, cells);

        for (size_t i = 0; i < objects.size(); ++i)
            resolve_wall_collision(objects[i], objects_speed[i]);

        check_obj_collision(objects, cells, objects_speed, collision_state);

        for (auto obj : objects) {
            DrawRectangleRec(obj, RED);
        }

        DrawLine(0, CELL_HEIGHT, GetScreenWidth(), CELL_HEIGHT, BLACK);
        DrawLine(0, CELL_HEIGHT*2, GetScreenWidth(), CELL_HEIGHT*2, BLACK);
        DrawLine(CELL_WIDTH, 0, CELL_WIDTH, GetScreenHeight(), BLACK);
        DrawLine(CELL_WIDTH*2, 0, CELL_WIDTH*2, GetScreenHeight(), BLACK);

        auto fps_text = TextFormat("Current Fps: (%i)", GetFPS());
        DrawText(fps_text, 10, 10, 20, DARKGRAY);

        EndDrawing();
    }

    CloseWindow();

    return 0;
}