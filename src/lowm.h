/**
 * Copyright (C) 2021, Eric Londo <londoed@comcast.net>, { include/lowm.h }
 * This software is distributed under the GNU General Public License Version 2.0.
 * See the file LICENSE for details.
**/
#ifndef LOWM_LOWM_H
#define LOWM_LOWM_H

#include "types.h"

#define WM_NAME "lowm"
#define CONFIG_NAME WM_NAME "rc"
#define CONFIG_HOME_ENV "XDG_CONFIG_HOME"
#define RUNTIME_DIR_ENV "XDG_RUNTIME_DIR"

#define STATE_PATH_IPL "/tmp/lowm%s_%i_%i-state"

#define ROOT_EVENT_MASK (XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT |             \
    XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY | XCB_EVENT_MASK_STRUCTURE_NOTIFY |  \
    XCB_EVENT_MASK_BUTTON_PRESS)
#define CLIENT_EVENT_MASK (XCB_EVENT_MASK_PROPERTY_CHANGE | XCB_EVENT_MASK_FOCUS_CHANGE)
#define LOWM_CLASS_NAME "Lowm"
#define META_WINDOW_IC "wm\0" LOWM_CLASS_NAME
#define ROOT_WINDOW_IC "root\0" LOWM_CLASS_NAME

#define PRESEL_FEEDBACK_I "presel_feedback"
#define PRESEL_FEEDBACK_IC PRESEL_FEEDBACK_I "\0" LOWM_CLASS_NAME
#define MOTION_RECORDER_I "motion_recorder"
#define MOTION_RECORDER_IC MOTION_RECORDER_I "\0" LOWM_CLASS_NAME

typedef struct {
    xcb_window_t id;
    uint16_t sequence;
    bool enabled;
} motion_recorder_t;

extern xcb_connection_t *dpy;
extern int default_screen, screen_width, screen_height;
extern uint32_t clients_count;
extern xcb_screen_t *screen;
extern xcb_window_t root;
extern char config_path[MAXLEN];

extern monitor_t *mon;
extern monitor_t *mon_head;
extern monitor_t *mon_tail;
extern monitor_t *pri_mon;

extern history_t *history_head;
extern history_t *history_tail;
extern history_t *history_needle;

extern rule_t *rule_head;
extern rule_t *rule_tail;

extern stacking_list_t *stack_head;
extern stacking_list_t *stach_tail;
extern subscriber_list_t *subscribe_head;
extern subscriber_list_t *subscribe_tail;
extern pending_rule_t *pending_rule_head;
extern pending_rule_t *pending_rule_tail;

extern xcb_window_t meta_window;
extern motion_recorder_t motion_recorder;

extern xcb_atom_t WM_STATE;
extern xcb_atom_t WM_TAKE_FOCUS;
extern xcb_atom_t WM_DELETE_WINDOW;

extern int exit_status;
extern bool auto_raise;
extern bool sticky_still;
extern bool record_history;
extern bool running;
extern bool restart;
extern bool randr;

void init(void);
void setup(void);
void register_events(void);
void cleanup(void);
bool check_connection(xcb_connection_t *dpy);
void sig_handler(int sig);
uint32_t get_color_pixel(const char *color);

#endif
