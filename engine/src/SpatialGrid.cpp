#include "engine/SpatialGrid.h"

#include <algorithm>
#include <cmath>

SpatialGrid::SpatialGrid(Rectangle area, size_t cell_count)
: area_(area),
  rows_(static_cast<int>(std::sqrt(static_cast<double>(cell_count)))),
  cell_width_(area.width / static_cast<float>(rows_)),
  cell_height_(area.height / static_cast<float>(rows_)),
  cells_(cell_count)
{
}

void SpatialGrid::clear()
{
    for (auto& cell : cells_)
        cell.clear();
}

void SpatialGrid::insert(size_t body_index, Rectangle bounds)
{
    const float left   = bounds.x - area_.x;
    const float top    = bounds.y - area_.y;
    const float right  = left + bounds.width;
    const float bottom = top + bounds.height;

    const int min_cell_x = std::clamp(static_cast<int>(left   / cell_width_),  0, rows_ - 1);
    const int max_cell_x = std::clamp(static_cast<int>(right  / cell_width_),  0, rows_ - 1);
    const int min_cell_y = std::clamp(static_cast<int>(top    / cell_height_), 0, rows_ - 1);
    const int max_cell_y = std::clamp(static_cast<int>(bottom / cell_height_), 0, rows_ - 1);

    for (int cell_x = min_cell_x; cell_x <= max_cell_x; ++cell_x) {
        for (int cell_y = min_cell_y; cell_y <= max_cell_y; ++cell_y) {
            cells_[cell_index(cell_x, cell_y)].push_back(body_index);
        }
    }
}

void SpatialGrid::rebuild(const std::vector<Body>& bodies)
{
    clear();

    for (size_t i = 0; i < bodies.size(); ++i)
        insert(i, bodies[i].bounds());
}

Rectangle SpatialGrid::cell_rect(size_t cell_index) const
{
    const size_t cell_x = cell_index / rows();
    const size_t cell_y = cell_index % rows();

    return {
        area_.x + cell_x * cell_width_,
        area_.y + cell_y * cell_height_,
        cell_width_,
        cell_height_
    };
}

size_t SpatialGrid::cell_index(int cell_x, int cell_y) const
{
    return static_cast<size_t>(cell_x * rows_ + cell_y);
}
