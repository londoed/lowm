/**
 * LOWM: An advanced tiling window manager for Unix.
 *
 * Copyright (C) 2021, Eric Londo <londoed@comcast.net, { include/events.h }.
 * This software is distributed under the GNU General Public License Version 2.0.
 * Refer to the file LICENSE for additional details.
**/

#ifndef LOWM_EVENTS_H
#define LOWM_EVENTS_H

#include <xcb/xcb.h>
#include <xcb/xcb_event.h>

#define ERROR_CODE_BAD_WINDOW 3

extern uint8_t randr_base;
static const xcb_button_index_t BUTTONS[] = {
	XCB_BUTTON_INDEX_1, XCB_BUTTON_INDEX_2, XCB_BUTTON_INDEX_3
};

void handle_event(xcb_generic_event_t *evt);
void map_request(xcb_generic_event_t *evt);
void configure_request(xcb_generic_event_t *evt);
void configure_notify(xcb_generic_event_t *evt);
void destroy_notify(xcb_generic_event_t *evt);
void unmap_notify(xcb_generic_event_t *evt);
void property_notify(xcb_generic_event_t *evt);
void client_message(xcb_generic_event_t *evt);
void focus_in(xcb_generic_event_t *evt);
void button_press(xcb_generic_event_t *evt);
void enter_notify(xcb_generic_event_t *evt);
void enter_notify(xcb_generic_event_t *evt);
void motion_notify(xcb_generic_event_t *evt);
void handle_state(monitor_t *m, desktop_t *d, node_t *n, xcb_atom_t state, unsigned int action);
void mapping_notify(xcb_generic_event_t *evt);
void process_error(xcb_generic_event_t *evt);

#endif
