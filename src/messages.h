/**
 * LOWM: An advanced tiling window manager for Unix.
 *
 * Copyright (C) 2021, Eric Londo <londoed@comcast.net>, { src/messages.h> }.
 * This software is distributed under the GNU General Public License Version 2.0.
 * Refer to the file LICENSE for additional details.
**/

#ifndef LOWM_MESSAGES_H
#define LOWM_MESSAGES_H

#include <lowm/types.h>
#include <lowm/subscribe.h>

void handle_message(char *msg, int msg_len, FILE *rsp);
void process_message(char **args, int num, FILE *rsp);
void cmd_node(char **args, int num, FILE *rsp);
void cmd_desktop(char **args, int num, FILE *rsp);
void cmd_monitor(char **args, int num, FILE *rsp);
void cmd_query(char **args, int num, FILE *rsp);
void cmd_rule(char **args, int num, FILE *rsp);
void cmd_wm(char **args, int num, FILE *rsp);
void cmd_subscribe(char **args, int num, FILE *rsp);
void cmd_quit(char **args, int num, FILE *rsp);
void cmd_config(char **args, int num, FILE *rsp);
void set_setting(coordinates_t loc, char *name, char *value, FILE *rsp);
void get_setting(coordinates_t loc, char *name, FILE *rsp);
void handle_failure(int code, char *src, char *val, FILE *rsp);
void fail(FILE *rsp, char *fmt, ...);

#endif
