#pragma once

#include "engine/Body.h"
#include "engine/SpatialGrid.h"

#include <cstddef>
#include <unordered_set>
#include <utility>
#include <vector>

// Cumulative counters since the system was created.
struct CollisionStats {
    size_t candidates = 0;   // pairs produced by the grid
    size_t duplicates = 0;   // pairs already tested this frame (shared cells)
    size_t collisions = 0;   // pairs that actually intersected

    size_t unique_candidates() const { return candidates - duplicates; }
};

// Walks the grid, filters duplicate pairs and resolves body-body contacts.
class CollisionSystem {
public:
    // Pre-size the pair set for this many bodies (optional, avoids rehashing).
    void reserve(size_t body_count);

    void resolve(std::vector<Body>& bodies, const SpatialGrid& grid);

    const CollisionStats& stats() const { return stats_; }

private:
    struct PairHash {
        size_t operator()(const std::pair<size_t, size_t>& p) const noexcept
        {
            return std::hash<size_t>{}(p.first)
                 ^ (std::hash<size_t>{}(p.second) << 1);
        }
    };

    std::unordered_set<std::pair<size_t, size_t>, PairHash> tested_pairs_;
    CollisionStats stats_;
};
