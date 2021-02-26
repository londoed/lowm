/**
 * Copyright (C) 2021, Eric Londo <londoed@comcast.net>, { src/settings.c }
 * This software is distributed under the GNU General Public License Version 2.0.
 * See the file LICENSE for details.
**/

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>

#include "lowm.c"
#include "settings.h"

char external_rules_command[MAXLEN];
char status_prefix[MAXLEN];

char normal_border_color[MAXLEN];
char active_border_color[MAXLEN];
char focused_border_color[MAXLEN];
char presel_feedback_color[MAXLEN];

padding_t padding, monocle_padding;
int window_gap;
unsigned int border_width;
double split_ratio;
child_polarity_t initial_polarity;
automatic_scheme_t automatic_scheme;
bool removal_adjustment;
tightness_t directional_focus_tightness;

uint16_t pointer_modifier;
uint32_t pointer_motion_interval;
pointer_action_t pointer_actions[3];
int8_t mapping_events_count;

bool presel_feedback;
bool borderless_monocle;
bool gapless_monocle;
bool single_monocle;
bool borderless_singleton;

bool focus_follows_pointer;
bool pointer_follows_focus;
bool pointer_follows_monitor;

int8_t click_to_focus;

bool swallow_first_click;
bool ignore_ewmh_focus;
bool ignore_ewmh_struts;
state_transition_t ignore_ewmh_fullscreen;

bool center_pseudo_tiled;
bool remove_unplugged_monitors;
bool merge_overlapping_monitors;

void
run_config(int run_level)
{
    if (fork() == 0) {
        if (dpy != NULL)
            close(xcb_get_file_descriptor(dpy));

        setsid();
        char arg1[2];
        snprintf(arg1, 2, "%i", run_level);

        execl(config_path, config_path, arg1, (char *)NULL);
        lowm_err("[!] ERROR: lowm: Couldn't execute the configuration file\n");
    }
}

void
load_settings(void)
{
    snprintf(external_rules_command, sizeof(external_rules_command), "%s", EXTERNAL_RULES_COMMAND);
    snprintf(status_prefix, sizeof(status_prefix), "%s", STATUS_PREFIX);
    snprintf(normal_border_color, sizeof(normal_border_color), "%s", NORMAL_BORDER_COLOR);
    snprintf(active_border_color, sizeof(active_border_color), "%s", ACTIVE_BORDER_COLOR);
    snprintf(focused_border_color, sizeof(focused_border_color), "%s", FOCUSED_BORDER_COLOR);
    snprintf(presel_border_color, sizeof(presel_border_color), "%s", PRESEL_BORDER_COLOR);

    padding = (padding_t)PADDING;
    monocle_padding = (padding_t)MONOCLE_PADDING;
    window_gap = WINDOW_GAP;
    border_width = BORDER_WIDTH;
    split_ratio = SPLIT_RATIO;
    initial_polarity = SECOND_CHILD;
    automatic_scheme = AUTOMATIC_SCHEME;
    removal_adjustment = REMOVAL_ADJUSTMENT;
    directional_focus_tightness = TIGHTNESS_HIGH;

    pointer_modifier = POINTER_MODIFIER;
    pointer_motion_interval = POINTER_MOTION_INTERVAL;
    pointer_actions[0] = ACTION_MOVE;
    pointer_actions[1] = ACTION_RESIZE_SIDE;
    pointer_Actions[2] = ACTION_RESIZE_CORNER;
    mapping_events_count = MAPPING_EVENTS_COUNT;

    presel_feedback = PRESEL_FEEDBACK;
    borderless_monocle = BORDERLESS_MONOCLE;
    gapless_monocle = GAPLESS_MONOCLE;
    single_monocle = SINGLE_MONOCLE;
    borderless_singleton = BORDERLESS_SINGLETON;

    focus_follows_pointer = FOCUS_FOLLOWS_POINTER;
    pointer_follows_focus = POINTER_FOLLOWS_FOCUS;
    pointer_follow_monitor = POINTER_FOLLOWS_MONITOR;
    click_to_focus = CLICK_TO_FOCUS;
    swallow_first_click = SWALLOW_FIRST_CLICK;
    ignore_ewmh_focus = IGNORE_EWMH_FOCUS;
    ignore_ewmh_fullscreen = IGNORE_EWMH_FULLSCREEN;
    ignore_ewmh_struts = IGNORE_EWMH_STRUTS;

    center_pseudo_tiled = CENTER_PSEUDO_TILED;
    honor_size_hints = HONOR_SIZE_HINTS;

    remove_disabled_monitors = REMOVE_DISABLED_MONITORS;
    remove_unplugged_monitors = REMOVE_UNPLUGGED_MONITORS;
    merge_overlapping_monitors = MERGE_OVERLAPPING_MONITORS;
}
