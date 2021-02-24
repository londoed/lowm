/**
 * Copyright (C) 2021, Eric Londo <londoed@comcast.net>, { include/ewmh.h }
 * This software is distributed under the GNU General Public License Version 2.0.
 * See the file LICENSE for details.
**/
#ifndef LOWM_EWMH_H
#define LOWM_EWMH_H

#include <xcb/xcb_ewmh.h>

extern xcb_ewmh_connection_t *ewmh;

void ewmh_init(void);
void ewmh_update_active_window(void);
void ewmh_update_number_of_desktops(void);
uint32_t ewmh_get_desktop_index(desktop_t *d);
bool ewmh_locate_desktop(uint32_t i, coordinates_t *loc);
void ewmh_update_current_desktop(void);
void ewmh_set_wm_desktop(node_t *n, desktop_t *d);
void ewmh_update_wm_desktops(void);
void ewmh_update_desktop_names(void);
void ewmh_update_desktop_viewport(void);
bool ewmh_handle_struts(xcb_window_t win);
void ewmh_update_client_list(bool stacking);
void ewmh_wm_state_update(node_t *n);
void ewmh_set_supporting(xcb_window_t win);

#endif
