/**
 * Copyright (C) 2021, Eric Londo <londoed@comcast.net>, { src/geometry.c }
 * This software is distributed under the GNU General Public License Version 2.0.
 * See the file LICENSE for details.
**/

#include <math.h>

#include "types.h"
#include "settings.h"
#include "geometry.h"

bool
is_inside(xcb_point_t p, xcb_rectangle_t r)
{
    return (p.x >= r.x && p.x < (r.x + r.width) &&
        p.y >= r.y && p.y < (r.y + r.height));
}

/* Returns true if `a` contains `b` */
bool
contains(xcb_rectangle_t a, xcb_rectangle_t b)
{
    return (a.x <= b.x && (a.x + a.width) >= (b.x + b.width) &&
        a.y <= b.y && (a.y + a.height) >= (b.y + b.height));
}

unsigned int
area(xcb_rectangle_t r)
{
    return r.width * r.height;
}

/* Distance between the `dir` edge of `r1` and the `opposite(dir)` edge of `r2` */
uint32_t
boundary_distance(xcb_rectangle_t r1, xcb_rectangle_t r2, direction_t dir)
{
    xcb_point_t r1_max = { r1.x + r1.width - 1, r1.y + r1.height - 1 };
    xcb_point_t r2_max = { r2.x + r2.width - 1, r2.y + r2.height - 1 };

    switch (dir) {
    case DIR_NORTH:
        return r2_max.y > r1.y ? r2_max.y - r1.y : r1.y - r2_max.y;
        break;

    case DIR_WEST:
        return r2_max.x > r1.x ? r2_max.x - r1.x : r1.x - r2_max.x;
        break;

    case DIR_SOUTH:
        return r2.y < r1_max.y ? r1_max.y - r2.y : r2.y - r1_max.y;
        break;

    case DIR_EAST:
        return r2.x < r1_max.x ? r1_max.x - r2.x : r2.x - r1_max.x;
        break;

    default:
        return UINT32_MAX;
    }
}

/* Is `r2` on the `dir` side of `r1`? */
bool
on_dir_side(xcb_rectangle_t r1, xcb_rectangle_t r2, direction_t dir)
{
    xcb_point_t r1_max = { r1.x + r1.width - 1, r1.y + r1.height - 1 };
    xcb_point_t r2_max = { r2.x + r2.width - 1, r2.y + r2.height - 1 };

    /* Eliminate rectangles on the opposite side */
    switch (directional_focus_tightness) {
    case TIGHTNESS_LOW:
        switch (dir) {
        case DIR_NORTH:
            if (r2.y > r1_max.y)
                return false;

            break;

        case DIR_WEST:
            if (r2.x > r1_max.x)
                return false;

            break;

        case DIR_SOUTH:
            if (r2_max.y < r1.y)
                return false;

            break;

        case DIR_EAST:
            if (r2_max.x < r1.x)
                return false;

            break;

        default:
            return false;
        }

        break;

    case TIGHTNESS_HIGH:
        switch (dir) {
        case DIR_NORTH:
            if (r2.y >= r1.y)
                return false;

            break;

        case DIR_WEST:
            if (r2.x >= r1.x)
                return false;

            break;

        case DIR_SOUTH:
            if (r2_max.y <= r1_max.y)
                return false;

            break;

        case DIR_EAST:
            if (r2_max.x <= r1_max.x)
                return false;

            break;

        default:
            return false;
        }

        break;

    default:
        return false;
    }

    /* Is there a shared vertical/horizontal range? */
    switch (dir) {
    case DIR_NORTH:
    case DIR_SOUTH:
        return (r2.x >= r1.x && r2.x <= r1_max.x) ||
            (r2_max.x >= r1.x && r2_max.x <= r1_max.x) ||
            (r1.x > r2.x && r1.x < r2_max.x);

    case DIR_WEST:
    case DIR_EAST:
        return (r2.y >= r1.y && r2.y <= r1_max.y) ||
            (r2_max.y >= r1.y && r2_max.y <= r1_max.y) ||
            (r1.y > r2.y && r1_max.y < r2_max.y);

        break;

    default:
        return false;
    }
}

bool
rect_eq(xcb_rectangle_t a, xcb_rectangle_t b)
{
    return (a.x == b.x && a.y == b.y && a.width == b.width && a.height == b.height);
}

int
rect_cmp(xcb_rectangle_t r1, xcb_rectangle_t r2)
{
    if (r1.y >= (r2.y + r2.height)) {
        return 1;
    } else if (r2.y >= (r2.y + r1.height)) {
        return -1;
    } else {
        if (r1.x >= (r2.x + r2.width))
            return 1;
        else if (r2.x >= (r1.x + r1.width))
            return -1;
        else
            return area(r2) - area(r1);
    }
}
