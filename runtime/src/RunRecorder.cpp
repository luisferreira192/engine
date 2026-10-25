#include "runtime/RunRecorder.h"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <limits>
#include <type_traits>
#include <utility>
#include <variant>

namespace {

constexpr int CSV_PRECISION = std::numeric_limits<float>::max_digits10;

// ---------------------------------------------------------
// Body log: one row per body per snapshot.
//
// Header and row are kept side by side; every field of Body must appear
// here. The assert below fails when Body's layout changes, as a reminder.
// ---------------------------------------------------------

static_assert(sizeof(Body) == 44,
              "Body changed: update write_body_header/write_body_row");

void write_body_header(std::ostream& out)
{
    out << "frame,time,id,"
           "collider,width,height,radius,"
           "pos_x,pos_y,vel_x,vel_y,"
           "mass,inv_mass,is_static,restitution,"
           "lock_x,lock_y\n";
}

void write_body_row(std::ostream& out, size_t frame, float time,
                    size_t id, const Body& body)
{
    out << frame << ',' << time << ',' << id << ',';

    // Collider-specific fields; ones that don't apply are left empty.
    std::visit([&](const auto& col) {
        using T = std::decay_t<decltype(col)>;

        if constexpr (std::is_same_v<T, BoxCollider>)
            out << "box," << col.width << ',' << col.height << ",,";
        else if constexpr (std::is_same_v<T, CircleCollider>)
            out << "circle,,," << col.radius << ',';
    }, body.collider);

    out << body.position.x << ',' << body.position.y << ','
        << body.velocity.x << ',' << body.velocity.y << ','
        << body.mass() << ',' << body.inv_mass() << ','
        << body.is_static() << ',' << body.restitution << ','
        << body.locked.x << ',' << body.locked.y << '\n';
}

// ---------------------------------------------------------
// Diagnostics log: one row per frame.
// ---------------------------------------------------------

void write_diagnostics_header(std::ostream& out)
{
    out << "frame,time,momentum_x,momentum_y,"
           "kinetic_energy,potential_energy,total_energy,dynamic_bodies\n";
}

void write_diagnostics_row(std::ostream& out, size_t frame, float time,
                           const WorldDiagnostics& d)
{
    out << frame << ',' << time << ','
        << d.momentum.x << ',' << d.momentum.y << ','
        << d.kinetic_energy << ',' << d.potential_energy << ','
        << d.kinetic_energy + d.potential_energy << ','
        << d.dynamic_bodies << '\n';
}

} // namespace

RunRecorder::RunRecorder(std::string root, size_t body_count, size_t cell_count)
: body_count_(body_count),
  cell_count_(cell_count),
  date_(timestamp()),
  // <root>/YYYY-MM-DD/HH-MM
  directory_(std::filesystem::path(std::move(root))
             / date_.substr(0, 10) / date_.substr(11, 5))
{
    candidate_output_
        << "candidates,duplicates,unique_candidates,actual_collisions\n";

    std::error_code ec;
    std::filesystem::create_directories(directory_, ec);
    if (ec)
        return;

    bodies_.open(file_path("bodies"));
    diagnostics_.open(file_path("diagnostics"));

    bodies_ << std::setprecision(CSV_PRECISION);
    diagnostics_ << std::setprecision(CSV_PRECISION);

    write_body_header(bodies_);
    write_diagnostics_header(diagnostics_);
}

bool RunRecorder::is_open() const
{
    return bodies_.is_open() && diagnostics_.is_open();
}

void RunRecorder::record_frame(size_t frame, float time, const World& world,
                               bool snapshot_bodies)
{
    write_diagnostics_row(diagnostics_, frame, time, world.diagnostics());

    if (!snapshot_bodies)
        return;

    const auto& bodies = world.bodies();

    for (size_t id = 0; id < bodies.size(); ++id)
        write_body_row(bodies_, frame, time, id, bodies[id]);
}

void RunRecorder::record_collision_stats(const CollisionStats& stats)
{
    candidate_output_
        << stats.candidates << ','
        << stats.duplicates << ','
        << stats.unique_candidates() << ','
        << stats.collisions << '\n';
}

bool RunRecorder::save(const Timer& timer)
{
    std::ofstream timelog(file_path("timelog"));
    std::ofstream candidates(file_path("candidatestats"));

    if (!timelog || !candidates)
        return false;

    timelog << timer.str();
    candidates << candidate_output_.str();

    bodies_.flush();
    diagnostics_.flush();

    return bodies_.good() && diagnostics_.good();
}

std::string RunRecorder::timestamp()
{
    auto now = std::chrono::system_clock::now();
    std::time_t time = std::chrono::system_clock::to_time_t(now);

    std::tm tm = *std::localtime(&time);

    std::ostringstream ss;
    ss << std::put_time(&tm, "%Y-%m-%d_%H-%M-%S");

    return ss.str();
}

std::filesystem::path RunRecorder::file_path(const std::string& prefix) const
{
    return directory_ / (prefix + "_"
                         + std::to_string(body_count_) + "_"
                         + std::to_string(cell_count_) + "_"
                         + date_ + ".csv");
}
