/**
 * Copyright (C) 2021, Eric Londo <londoed@comcast.net>, { include/monitor.h }
 * This software is distributed under the GNU General Public License Version 2.0.
 * See the file LICENSE for details.
**/
#ifndef LOWM_MONITOR_H
#define LOWM_MONITOR_H

#define DEFAULT_MON_NAME "MONITOR"

monitor_t *make_monitor(const char *name, xcb_rectangle_t *rect, uint32_t id);
void update_root(monitor_t *m, xcb_rectangle_t *rect);
void reorder_monitor(monitor_t *m);
void rename_monitor(monitor_t *m, const char *name);
monitor_t *find_monitor(uint32_t id);
monitor_t *get_monitor_by_randr_id(xcb_randr_output_t id);
void embrace_client(monitor_t *m, client_t *c);
void adapt_geometry(xcb_rectangle_t *rs, xcb_rectangle_t *rd, node_t *n);
void add_monitor(monitor_t *m);
void unlink_monitor(monitor_t *m);
void remove_monitor(monitor_t *m);
void merge_monitors(monitor_t *ms, monitor_t *md);
bool swap_monitor(monitor_t *m1, monitor_t *m2);
monitor_t *closest_monitor(monitor_t *m, cycle_dir_t dir, monitor_select_t *sel);
bool is_inside_monitor(monitor_t *m, xcb_point_t pt);
monitor_t *monitor_from_point(xcb_point_t pt);
monitor_t *monitor_from_client(client_t *c);
monitor_t *nearest_monitor(monitor_t *m, direction_t dir, monitor_select_t *sel);
bool find_any_monitor(coordinates_t *ref, coordinates_t *dst, monitor_select_t *sel);
bool update_monitors(void);

#endif
