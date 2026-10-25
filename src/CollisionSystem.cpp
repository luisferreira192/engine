#include "CollisionSystem.h"
#include "Collision.h"

#include <algorithm>

void CollisionSystem::reserve(size_t body_count)
{
    if (body_count > 1)
        tested_pairs_.reserve((body_count * (body_count - 1)) / 2);
}

void CollisionSystem::resolve(std::vector<Body>& bodies, const SpatialGrid& grid)
{
    tested_pairs_.clear();

    for (const auto& cell : grid.cells()) {
        for (size_t i = 0; i < cell.size(); ++i) {
            for (size_t j = i + 1; j < cell.size(); ++j) {

                ++stats_.candidates;

                auto pair = std::minmax(cell[i], cell[j]);

                // Bodies spanning several cells show up more than once.
                if (!tested_pairs_.insert(pair).second) {
                    ++stats_.duplicates;
                    continue;
                }

                Body& a = bodies[pair.first];
                Body& b = bodies[pair.second];

                if (!collision::intersects(a, b))
                    continue;

                ++stats_.collisions;

                collision::resolve(a, b, collision::contact(a, b));
            }
        }
    }
}
