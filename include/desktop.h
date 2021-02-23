/**
 * Copyright (C) 2021, Eric Londo <londoed@comcast.net>, { include/dektop.h }
 * This software is distributed under the GNU General Public License Version 2.0.
 * See the file LICENSE for details.
**/
#ifndef LOWM_DESKTOP_H
#define LOWM_DESKTOP_H

#define DEFAULT_DESK_NAME "Desktop"

bool activate_desktop(monitor_t *m, desktop_t *d);
bool find_closest_desktop(coordinates_t *ref, coordinates_t *dst, cycle_dir_t dir,
    desktop_select_t *sel);
bool find_any_desktop(coordinates_t *ref, coordinates_t *dst, desktop_select_t *sel);
bool set_layout(monitor_t *m, desktop_t *d, layout_t l, bool user);
void handle_presel_feedbacks(monitor_t *m, desktop_t *d);
bool transfer_desktop(monitor_t *ms, monitor_t *md, desktop_t, bool follow);
desktop_t *make_desktop(const char *name, uint32_t id);
void rename_desktop(monitor_t *m, desktop_t *d, const char *name);
void insert_desktop(monitor_t *m, desktop_t *d);
void add_desktop(monitor_t *m, desktop_t *d);
desktop_t *find_desktop_in(uint32_t id, monitor_t *m);
void unlink_desktop(monitor_t *m, desktop_t *d);
void remove_desktop(monitor_t *m, desktop_t *d);
void merge_desktops(monitor_t *m, desktop_t *ds, monitor_t *md, desktop_t *dd);
bool swap_desktops(monitor_t *m1, desktop_t *d1, monitor_t *m2, desktop_t *d2, bool follow);
void show_desktop(desktop_t *d);
void hide_desktop(desktop_t *d);
bool is_urgent(desktop_t *d);

#endif
