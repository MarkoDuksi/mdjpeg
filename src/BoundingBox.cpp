#include "BoundingBox.h"

#include <cmath>
#include <algorithm>


void BoundingBox::merge(const BoundingBox &other) noexcept
{

    topleft_X = std::min(topleft_X, other.topleft_X);
    topleft_Y = std::min(topleft_Y, other.topleft_Y);

    bottomright_X = std::max(bottomright_X, other.bottomright_X);
    bottomright_Y = std::max(bottomright_Y, other.bottomright_Y);
}

bool BoundingBox::expand_to_square(const BoundingBox& outer_bounds) noexcept {

    // if expanding would grow the bounding box outside of `outer_bounds`
    if (std::max(width(), height()) > std::min(outer_bounds.width(), outer_bounds.height())) {

        return false;
    }

    // if width should match height
    if (width() < height()) {

        topleft_X = std::max(0, static_cast<int>(topleft_X - std::lround((height() - width()) / 2.0f)));
        bottomright_X = std::min(static_cast<int>(outer_bounds.bottomright_X), topleft_X + height());
        topleft_X = bottomright_X - height();
    }

    // if height should match width
    else if (height() < width()) {

        topleft_Y = std::max(0, static_cast<int>(topleft_Y - std::lround((width() - height()) / 2.0f)));
        bottomright_Y = std::min(static_cast<int>(outer_bounds.bottomright_Y), topleft_Y + width());
        topleft_Y = bottomright_Y - width();
    }

    return true;
}
