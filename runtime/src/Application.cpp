#include "runtime/Application.h"

#include <iostream>

// ---------------------------------------------------------
// Window
// ---------------------------------------------------------

Window::Window(int width, int height, const char* title, int target_fps)
{
    InitWindow(width, height, title);
    SetTargetFPS(target_fps);
}

Window::~Window()
{
    CloseWindow();
}

bool Window::should_close() const
{
    return WindowShouldClose();
}

// ---------------------------------------------------------
// Application
// ---------------------------------------------------------

Application::Application(const SimulationConfig& config, Scenario& scenario)
: config_(config),
  window_(config_.screen_width, config_.screen_height, scenario.name(), config_.target_fps),
  world_(build_world(config_, scenario)),
  recorder_(config_.output_root, world_.bodies().size(), config_.cell_count)
{
    if (!recorder_.is_open())
        std::cerr << "Failed to open log files in " << recorder_.directory() << '\n';

    // Initial conditions.
    recorder_.record_frame(0, 0.0f, world_, config_.body_log_interval > 0);
}

World Application::build_world(const SimulationConfig& config, Scenario& scenario)
{
    World world(
        Rectangle{
            0.0f,
            0.0f,
            static_cast<float>(config.screen_width),
            static_cast<float>(config.screen_height)
        },
        config.cell_count,
        scenario.world_settings()
    );

    scenario.build(world, config);
    world.prepare_simulation();

    return world;
}

int Application::run()
{
    while (!window_.should_close() && frame_ <= config_.max_frames) {
        timer_.start();

        float dt = GetFrameTime();
        step_simulation(dt);
        render();

        timer_.end("frame");

        sim_time_ += dt;
        end_frame();
    }

    // Save any frames left over after the last full block.
    if (timer_.frame_count() > 0)
        timer_.write_stats();

    if (!recorder_.save(timer_)) {
        std::cerr << "Failed to write run statistics to "
                  << recorder_.directory() << '\n';
        return 1;
    }

    std::cout << "Run logged to " << recorder_.directory() << '\n';

    return 0;
}

void Application::step_simulation(float dt)
{
    world_.integrate(dt);
    timer_.lap_no_print("move");

    world_.resolve_wall_collisions();
    timer_.lap_no_print("wall");

    world_.update_grid();
    timer_.lap_no_print("grid");

    world_.resolve_body_collisions();
    timer_.lap_no_print("collision");
}

void Application::render()
{
    BeginDrawing();
    timer_.lap_no_print("BeginDrawing");

    ClearBackground(renderer_.background);
    timer_.lap_no_print("ClearBackground");

    renderer_.draw_world(world_);
    timer_.lap_no_print("draw objects");

    EndDrawing();
    timer_.lap_no_print("EndDrawing");
}

// Bookkeeping outside the timed section, so logging doesn't skew timings.
void Application::end_frame()
{
    ++frame_;

    const bool snapshot_bodies =
        config_.body_log_interval > 0 && frame_ % config_.body_log_interval == 0;

    recorder_.record_frame(frame_, sim_time_, world_, snapshot_bodies);

    if (frame_ % config_.stats_interval == 0)
        recorder_.record_collision_stats(world_.collision_stats());

    if (timer_.frame_count() >= config_.stats_interval) {
        timer_.write_stats();
        timer_.reset_stats();
    }
}
