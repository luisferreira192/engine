#pragma once

#include <algorithm>
#include <chrono>
#include <fstream>
#include <limits>
#include <sstream>
#include <string>
#include <unordered_map>
#include <iostream>

class Timer {
public:
    using Clock = std::chrono::high_resolution_clock;
    using TimePoint = Clock::time_point;

    Timer() 
    {
        output << "metric,avg_us,min_us,max_us,samples\n";
    }

    struct Stats {
        double total_us = 0.0;
        double min_us = std::numeric_limits<double>::max();
        double max_us = 0.0;
        std::size_t samples = 0;

        void add(double us)
        {
            total_us += us;
            min_us = std::min(min_us, us);
            max_us = std::max(max_us, us);
            ++samples;
        }

        double average() const
        {
            return samples > 0
                ? total_us / static_cast<double>(samples)
                : 0.0;
        }
    };

    void start()
    {
        start_time = Clock::now();
        last_lap = start_time;
    }

    double lap(const std::string& name)
    {
        double us = lap_no_print(name);

        output
            << name << ": "
            << us << " us\n";

        return us;
    }

    double lap_no_print(const std::string& name)
    {
        auto now = Clock::now();

        double us =
            std::chrono::duration<double, std::micro>(
                now - last_lap
            ).count();

        last_lap = now;

        stats[name].add(us);

        return us;
    }

    double end(const std::string& name = "frame")
    {
        auto now = Clock::now();

        double us =
            std::chrono::duration<double, std::micro>(
                now - start_time
            ).count();

        stats[name].add(us);
        ++frames;

        return us;
    }

    void write_stats()
    {
        for (const auto& [name, s] : stats) {
            output
                << name << ','
                << s.average() << ','
                << s.min_us << ','
                << s.max_us << ','
                << s.samples << "\n";
        }

        output << '\n';
    }

    void reset_stats()
    {
        stats.clear();
        frames = 0;
    }

    std::size_t frame_count() const
    {
        return frames;
    }

    void mark()
    {
        last_lap = Clock::now();
    }

    std::string str() const
    {
        return output.str();
    }

    void clear_output()
    {
        output.str("");
        output.clear();
    }

    bool dump_to_file(const std::string& filename) const
    {
        std::ofstream file(filename);

        /*if (!file)
            return false;*/

        file << output.str();

        return true;
    }

private:
    TimePoint start_time{};
    TimePoint last_lap{};

    std::unordered_map<std::string, Stats> stats{};

    std::size_t frames = 0;

    std::ostringstream output{};
};