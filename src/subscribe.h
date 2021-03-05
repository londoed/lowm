/**
 * LOWM: An advanced tiling window manager for Unix.
 *
 * Copyright (C) 2021, Eric Londo <londoed@comcast.net>, { src/subscribe.h }.
 * This software is distributed under the GNU General Public License Version 2.0.
 * Refer to the file LICENSE for additional details.
**/

#ifndef LOWM_SUBSCRIBE_H
#define LOWM_SUBSCRIBE_H

#define FIFO_TEMPLATE "lowm_fifo.XXXXXX"

typedef enum {
	SBSC_MASK_REPORT = 1 << 0,
	SBSC_MASK_MONITOR_ADD = 1 << 1,
	SBSC_MASK_MONITOR_RENAME = 1 << 2,
	SBSC_MASK_MONITOR_REMOVE = 1 << 3,
	SBSC_MASK_MONITOR_SWAP = 1 << 4,
	SBSC_MASK_MONITOR_FOCUS = 1 << 5,
	SBSC_MASK_MONITOR_GEOMETRY = 1 << 6,
	SBSC_MASK_DESKTOP_ADD = 1 << 7,
	SBSC_MASK_DESKTOP_RENAME = 1 << 8,
	SBSC_MASK_DESKTOP_REMOVE = 1 << 9,
	SBSC_MASK_DESKTOP_SWAP = 1 << 10,
	SBSC_MASK_DESKTOP_TRANSFER = 1 << 11,
	SBSC_MASK_DESKTOP_FOCUS = 1 << 12,
	SBSC_MASK_DESKTOP_ACTIVATE = 1 << 13,
	SBSC_MASK_DESKTOP_LAYOUT = 1 << 14,
	SBSC_MASK_NODE_ADD = 1 << 15,
	SBSC_MASK_NODE_REMOVE = 1 << 16,
	SBSC_MASK_NODE_SWAP = 1 << 17,
	SBSC_MASK_NODE_TRANSFER = 1 << 18,
	SBSC_MASK_NODE_FOCUS = 1 << 19,
	SBSC_MASK_NODE_PRESEL = 1 << 20,
	SBSC_MASK_NODE_STACK = 1 << 21,
	SBSC_MASK_NODE_ACTIVATE = 1 << 22,
	SBSC_MASK_NODE_GEOMETRY = 1 << 23,
	SBSC_MASK_NODE_STATE = 1 << 24,
	SBSC_MASK_NODE_FLAG = 1 << 25,
	SBSC_MASK_NODE_LAYER = 1 << 26,
	SBSC_MASK_POINTER_ACTION = 1 << 27,
	SBSC_MASK_MONITOR = (1 << 7) - (1 << 1),
	SBSC_MASK_DESKTOP = (1 << 15) - (1 << 7),
	SBSC_MASK_NODE = (1 << 28) - (1 << 15),
	SBSC_MASK_ALL = (1 << 28) - 1,
} subscriber_mask_t;

subscriber_list_t *make_subscriber(FILE *stream, char *fifo_path, int field, int count);
void remove_subscriber(subscriber_list_t *sb);
void add_subscriber(subscriber_list_t *sb);
int print_report(FILE *stream);
void put_status(subscriber_mask_t mask, ...);

/**
 * Remove any subscriber for which the stream has been closed an
 * is no longer writable.
**/
void prune_dead_subscribers(void);

#endif
