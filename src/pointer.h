/**
 * LOWM: An advanced tiling window manager for Unix.
 *
 * Copyright (C) 2021, Eric Londo <londoed@comcast.net>, { src/pointer.h }.
 * This software is distributed under the GNU General Public License Version 2.0.
 * Refer to the file LICENSE for additional details.
**/

#ifndef LOWM_POINTER_H
#define LOWM_POINTER_H

#define XK_Num_Lock 0xff7f
#define XK_Caps_Lock 0xffe5
#define XK_Scroll_Lock 0xff14

extern uint16_t num_lock;
extern uint16_t caps_lock;
extern uint16_t scroll_lock;
extern bool grabbing;
extern node_t *grabbed_node;

void pointer_init(void);
void window_grab_buttons(xcb_window_t win);
void window_grab_button(xcb_window_t win, uint8_t button, uint16_t modifier);
void grab_buttons(void);
void ungrab_buttons(void);
int16_t modfield_from_keysym(xcb_keysym_t keysym);
resize_handle_t get_handle(node_t *n, xcb_point_t pos, pointer_action_t pac);
bool grab_pointer(pointer_action_t pac);
void track_pointer(coordinates_t loc, pointer_action_t pac, xcb_point_t pos);

#endif
