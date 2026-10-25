#pragma once

#include "runtime/Config.h"
#include "runtime/Renderer.h"
#include "runtime/RunRecorder.h"
#include "runtime/Scenario.h"
#include "runtime/Timer.h"
#include "engine/World.h"

#include <cstddef>

// RAII wrapper so the window is opened before anything that depends on it
// (raylib seeds its RNG in InitWindow) and closed on destruction.
class Window {
public:
    Window(int width, int height, const char* title, int target_fps);
    ~Window();

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    bool should_close() const;
};

class Application {
public:
    // config should already have been passed through scenario.configure().
    Application(const SimulationConfig& config, Scenario& scenario);

    int run();

private:
    static World build_world(const SimulationConfig& config, Scenario& scenario);

    void step_simulation(float dt);
    void render();
    void end_frame();

    SimulationConfig config_;

    // Declaration order matters: the window must exist before the world
    // is populated, and the recorder needs the final body count.
    Window window_;
    World world_;
    Renderer renderer_;
    Timer timer_;
    RunRecorder recorder_;

    size_t frame_ = 0;       // simulation steps taken
    float sim_time_ = 0.0f;  // seconds simulated
};
