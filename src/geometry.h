/**
 * LOWM: An advanced tiling window manager for Unix.
 *
 * Copyright (C) 2021, Eric Londo <londoed@comcast.net>, { include/geometry.h }.
 *
 * This software is distributed under the GNU General Public License Version 2.0.
 * Refer to the file LICENSE for additional details.
**/

#ifndef LOWM_GEOMETRY_H
#define LOWM_GEOMETRY_H

#include <stdbool.h>
#include <xcb/xcb.h>

bool is_inside(xcb_point_t p, xcb_rectangle_t r);
bool contains(xcb_rectangle_t a, xcb_rectangle_t b);
unsigned int area(xcb_rectangle_t r);
uint32_t boundary_distance(xcb_rectangle_t r1, xcb_rectangle_t r2, direction_t dir);
bool on_dir_side(xcb_rectangle_t a, xcb_rectangle_t b);
bool rect_eq(xcb_rectangle_t a, xcb_rectangle_t b);
int rect_cmp(xcb_rectangle_t r1, xcb_rectangle_t r2);

#endif
