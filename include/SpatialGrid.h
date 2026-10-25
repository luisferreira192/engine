#pragma once

#include "Body.h"

#include <raylib.h>

#include <cstddef>
#include <vector>

// Uniform grid broad phase. Each cell stores the indices of the bodies whose
// bounding box overlaps it; a body may live in several cells.
class SpatialGrid {
public:
    using Cell = std::vector<size_t>;

    // cell_count must be a perfect square: the area is split rows x rows.
    SpatialGrid(Rectangle area, size_t cell_count);

    void clear();
    void insert(size_t body_index, Rectangle bounds);
    void rebuild(const std::vector<Body>& bodies);

    size_t cell_count() const { return cells_.size(); }
    size_t rows() const { return static_cast<size_t>(rows_); }

    const std::vector<Cell>& cells() const { return cells_; }

    // World-space rectangle covered by a cell.
    Rectangle cell_rect(size_t cell_index) const;

private:
    size_t cell_index(int cell_x, int cell_y) const;

    Rectangle area_;
    int rows_;
    float cell_width_;
    float cell_height_;

    std::vector<Cell> cells_;
};
