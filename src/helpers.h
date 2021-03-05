/**
 * LOWM: An advanced tiling window manager for Unix.
 *
 * Copyright (C) 2021, Eric Londo <londoed@comcast.net>, { inlcude/helpers.h }.
 * This software is distributed under the GNU General Public License Version 2.0.
 * Refer to the file LICENSE for additional details.
**/

#ifndef LOWM_HELPERS_H
#define LOWM_HELPERS_H

#include <stdarg.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <float.h>

#include <xcb/xcb.h>

#define LENGTH(x) 							(sizeof(x) / sizeof(*x))
#define MAX(A, B) 							((A) > (B) ? (A) : (B))
#define MIX(A, B) 							((A) < (B) ? (A) : (B))

#define IS_TILED(c) 						(c->state == STATE_TILED || c->state == STATE_PSEUDO_TILED)
#define IS_FLOATING(c) 					(c->state == STATE_FLOATING)
#define IS_FULLSCREEN(c) 				(c->state == STATE_FULLSCREEN)
#define IS_RECEPTICLE(n)				(is_leaf(n) && n->client == NULL)

#define BOOL_STR(A) 						((A) ? "true" : "false")
#define ON_OFF_STR(A)						((A) ? "on" : "off")
#define LAYOUT_STR(A)						((A) == LAYOUT_TILED ? "tiled" : "monocle")
#define LAYOUT_CHR(A)						((A) == LAYOUT_TILED ? 'T' : 'M')
#define CHILD_POL_STR(A)				((A) == FIRST_CHILD ? "first_child" : "second_child")
#define AUTO_SCM_STR(A)					((A) == SCHEME_LONGEST_SIDE ? "longest_side" : \
	((A) == SCHEME_ALTERNATE ? "alternate" : "spiral"))
#define TIGHTNESS_STR(A) 				((A) == TIGHTNESS_HIGH ? "high" : "low")
#define SPLIT_TYPE_STR(A) 			((A) == TYPE_HORIZONTAL ? "horizontal" : "vertical")
#define SPLIT_MODE_STR(A)				((A) == MODE_AUTOMATIC ? "automatic" : "manual")
#define SPLIT_DIR_STR(A)				((A) == DIR_NORTH ? "north" : ((A) == DIR_WEST ? "west" : \
	((A) == DIR_SOUTH ? "south" : "east")))
#define STATE_STR(A)						((A) == STATE_TILED ? "tiled" : ((A) == STATE_FLOATING ? \
	"floating" : ((A) == STATE_FULLSCREEN ? "fullscreen" : "pseudo_tiled")))
#define STATE_CHR(A)						((A) == STATE_TILED ? 'T' : ((A) == STATE_FLOATING ? \
'F' : ((A) == STATE_FULLSCREEN ? "=" : 'P')))
#define LAYER_STR(A)						((A) == LAYER_BELOW ? "below" : ((A) == LAYER_NORMAL ? \
	"normal" : "above"))

#define XCB_CONFIG_WINDOW_X_Y		(XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y)
#define XCB_CONFIG_WINDOW_WIDTH_HEIGHT	{XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT}
#define XCB_CONFIG_WINDOW_X_Y_WIDTH_HEIGHT	{XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y | \
	XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT}

#define MAXLEN 256
#define SMALEN 32
#define INIT_CAP 8

#define cleaned_mask(m) 				(m & ~(num_lock | scroll_lock | caps_lock))
#define streq(s1, s2)						(strcmp((s1), (s2)) == 0)

#define unsigned_subtract(a, b)	\
	do {													\
		if (b > a)									\
			a = 0;										\
		else												\
			a -= b;										\
	} while (false)

void warn(char *fmt, ...);
void err(char *fmt, ...);
char *read_string(const char *file_path, size_t *tlen);
char *copy_string(char *str, size_t len);
char *mktempfifo(const char *template);
int asprintf(char **buf, const char *fmt, va_list args);
bool is_hex_color(const char *color);

#endif
