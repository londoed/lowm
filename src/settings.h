/**
 * LOWM: An advanced tiling window manager for Unix.
 *
 * Copyright (C) 2021, Eric Londo <londoed@comcast.net>, { src/settings.h }.
 * This software is distributed under the GNU General Public License Version 2.0.
 * Refer to the file LICENSE for additional details.
**/

#ifndef LOWM_SETTINGS_H
#define LOWM_SETTINGS_H

#include <types.h>

#define POINTER_MODIFIER XCB_MOD_MASK_4
#define POINTER_MOTION_INTERVAL 17
#define EXTERNAL_RULES_COMMAND ""
#define STATUS_PREFIX "W"

#define NORMAL_BORDER_COLOR "#30302f"
#define ACTIVE_BORDER_COLOR "#474645"
#define FOCUSED_BORDER_COLOR "#817f7f"
#define PRESEL_FEEDBACK_COLOR "#f4d775"

#define PADDING { 0, 0, 0, 0}
#define MONOCLE_PADDING { 0, 0, 0, 0 }
#define WINDOW_GAP 6
#define BORDER_WIDTH 1
#define SPLIT_RATIO 0.5
#define AUTOMATIC_SCHEME SCHEME_LONGEST_SIDE
#define REMOVAL_ADJUSTMENT true

#define PRESEL_FEEDBACK true
#define BORDERLESS_MONOCLE false
#define GAPLESS_MONOCLE false
#define SINGLE_MONOCLE false
#define BORDERLESS_SINGLETON false

#define FOCUS_FOLLOWING_POINTER false
#define POINTER_FOLLOWS_FOCUS false
#define POINTER_FOLLOWS_MONITOR false
#define CLICK_TO_FOCUS XCB_BUTTON_INDEX_1
#define SWALLOW_FIRST_CLICK false
#define IGNORE_EWMH_FOCUS false
#define IGNORE_EWMH_FULLSCREEN 0
#define IGNORE_EWMH_STRUTS false

#define CENTER_PSEUDO_TILED true
#define HONOR_SIZE_HINTS false
#define MAPPING_EVENTS_COUNT 1

#define REMOVE_DISABLED_MONITORS false
#define REMOVE_UNPLUGGED_MONITORS false
#define MERGE_OVERLAPPING_MONITORS false

extern char external_rules_command[MAXLEN];
extern char status_prefix[MAXLEN];
extern char normal_border_color[MAXLEN];
extern char active_border_color[MAXLEN];
extern char focused_border_color[MAXLEN];
extern char presel_feedback_color[MAXLEN];

extern padding_t padding;
extern padding_t monocle_padding;
extern int window_gap;
extern unsigned int border_width;
extern double split_ratio;
extern child_polarity_t initial_polarity;
extern automatic_scheme_t automatic_scheme;
extern bool removal_adjustment;
extern tightness_t directional_focus_tightness;

extern uint16_t pointer_modifier;
extern uint32_t pointer_motion_interval;
extern pointer_action_t pointer_actions[3];
extern int8_t mapping_events_count;

extern bool presel_feedback;
extern bool borderless_monocle;
extern bool gapless_monocle;
extern bool single_monocle;
extern bool borderless_singleton;

extern bool focus_follows_pointer;
extern bool pointer_follows_focus;
extern bool pointer_follows_monitor;
extern int8_t click_to_focus;
extern bool swallow_first_click;
extern bool ignore_ewmh_focus;
extern bool ignore_ewmh_struts;
extern state_transition_t ignore_ewmh_fullscreen;

extern bool center_pseudo_tiled;
extern bool honor_size_hint;
extern bool remove_disabled_monitors;
extern bool remove_unplugged_monitors;
extern bool merge_overlapping_monitors;

void run_config(int run_level);
void load_settings(void);

#endif
