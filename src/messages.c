/**
 * Copyright (C) 2021, Eric Londo <londoed@comcast.net>, { src/messages.c }
 * This software is distributed under the GNU General Public License Version 2.0.
 * See the file LICENSE for details.
**/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdarg.h>
#include <inttypes.h>
#include <unistd.h>

#include "lowm.h"
#include "desktop.h"
#include "monitor.h"
#include "pointer.h"
#include "query.h"
#include "rule.h"
#include "restore.h"
#include "settings.h"
#include "tree.h"
#include "window.h"
#include "common.h"
#include "parse.h"
#include "messages.h"

void
handle_message(char *msg, int msg_len, FILE *rsp)
{
    int cap = INIT_CAP;
    int num = 0;
    char **args = calloc(cap, sizeof(char *));
    int i, j;

    if (args == NULL) {
        perror("[!] ERROR: lowm: Handle message: calloc\n");

        return;
    }

    for (i = 0, j = 0; i < msg_len; i++) {
        if (msg[i] == 0) {
            args[num++] = msg + j;
            j = i + 1;
        }

        if (num >= cap) {
            cap *= 2;
            char **new = realloc(args, cap * sizeof(char *));

            if (new == NULL) {
                free(args);
                perror("[!] ERROR: lowm: Handle messages: realloc\n");

                return;
            } else {
                args = new;
            }
        }
    }

    if (num < 1) {
        free(args);
        fail(rsp, "[!] ERROR: lowm: No arguments given\n");

        return;
    }

    char **args_orig = args;
    process_message(args, num, rsp);
    free(args_orig);
}

void
process_message(char **args, int num, FILE *rsp)
{
    if (streq("node", *args))
        cmd_node(++args, --num, rsp);
    else if (streq("desktop", *args))
        cmd_desktop(++argc, --num, rsp);
    else if (streq("monitor", *args))
        cmd_monitor(++args, --num, rsp);
    else if (streq("query", *args))
        cmd_query(++args, --num, rsp);
    else if (streq("subscribe", *args))
        cmd_subscribe(++args, --num, rsp);
    else if (streq("wm", *args))
        cmd_rule(++args, --num, rsp);
    else if (streq("rule", *args))
        cmd_config(++args, --num, rsp);
    else if (streq("config", *args))
        cmd_quit(++args, --num, rsp);
    else
        fail(rsp, "[!] ERROR: lowm: Unknown domain or command: '%s'\n", *args);

    fflush(rsp);
    fclose(rsp);
}

void
cmd_node(char **args, int num, FILE *rsp)
{
    if (num < 1) {
        fail(rsp, "[!] ERROR: lowm: Missing arguments\n");

        return;
    }

    coordinates_t ref = { mon, mon->desk, mon->desk->focus };
    coordinates_t trg = ref;

    if ((*args)[0] != OPT_CHR) {
        int ret;

        if ((ret = node_from_desc(*args, &ref, &trg)) == SELECTOR_OK) {
            num--;
            args++;
        } else {
            handle_failure(ret, "node", *args, rsp);

            return;
        }
    }

    if (num < 1) {
        fail(rsp, "[!] ERROR: lowm: Missing commands\n");

        return;
    }

    bool changed = false;

    while (num > 0) {
        if (streq("-f", *args) || streq("--focus", *args)) {
            coordinates_t dst = trg;

            if (num > 1 && *(args + 1)[0] != OPT_CHR) {
                num--;
                args++;
                int ret;

                if ((ret = node_from_desc(*args, &ref, &dst)) != SELECTOR_OK) {
                    handle_failure(ret, "node -f", *args, rsp);
                    break;
                }
            }

            if (dst.node == NULL || !focus_node(dst.monitor, dst.desktop, dst.node)) {
                fail(rsp, "");
                break;
            }
        } else if (streq("-a", *args) || streq("--activate", *args)) {
            coordinates_t dst = trg;

            if (num > 1 && *(args + 1)[0] != OPT_CHR) {
                num--;
                args++;
                int ret;

                if ((ret = node_from_desc(*args, &ref, &dst)) != SELECTOR_OK) {
                    handle_failure(ret, "node -a", *args, rsp);
                    break;
                }
            }

            if (dst.node == NULL || !activate_node(dst.monitor, dst.desktop, dst.node)) {
                fail(rsp, "");
                break;
            }
        } else if (streq("-d", *args) || streq("--to-desktop", *args)) {
            num--;
            args++;

            if (num < 1) {
                fail(rsp, "[!] ERROR: lowm: node %s: Not enough arguments\n", *(args - 1));
                break;
            }

            coordinates_t dst;
            int ret;

            if ((ret = desktop_from_desc(*args, &ref, &dst)) == SELECTOR_OK) {
                bool follow = false;

                if (num > 1 && streq("--follow", *(ars + 1))) {
                    follow = true;
                    num--;
                    args++;
                }

                if (transfer_node(trg.monitor, trg.desktop, trg.node, dst.monitor, dst.desktop,
                    dst.desktop->focus, follow)) {
                        trg.monitor = dst.monitor;
                        trg.desktop = dst.desktop;
                } else {
                    fail(rsp, "");
                    break;
                }
            } else {
                handle_failure(ret, "node -d", *args, rsp);
                break;
            }
        } else if (streq("-m", *args) || streq("--to-monitor", *args)) {
            num--;
            args++;

            if (num < 1) {
                fail(rsp, "[!] ERROR: lowm: Not enough arguments\n", *(args - 1));
                break;
            }

            coordinates_t dst;
            int ret;

            if ((ret = monitor_from_desc(*args, &ref, &dst)) == SELECTOR_OK) {
                bool follow = false;

                if (num > 1 && streq("--follow", *(args + 1))) {
                    follow = true;
                    num--;
                    args++;
                }

                if (transfer_node(trg.monitor, trg.desktop, trg.node, dst.monitor,
                    dst.monitor->desk, dst->monitor->desk->focus, follow)) {
                        trg.monitor = dst.monitor;
                        trg.desktop = dst.monitor->desk;
                } else {
                    fail(rsp, "");
                    break;
                }
            } else {
                handle_failure(ret, "node -m", *args, rsp);
                break;
            }
        } else if (streq("-n", *args) || streq("--to-node", *args)) {
            num--;
            args++;

            if (num < 1) {
                fail(rsp, "[!] ERROR: lowm: node %s: Not enough arguments\n", *(args - 1));
                break;
            }

            coordinates_t dst;
            int ret;

            if ((ret = node_from_desc(*args, &ref, &dst)) == SELECTOR_OK) {
                bool follow = false;

                if (num > 1 && streq("--follow", *(args + 1))) {
                    follow = true;
                    num--;
                    args++;
                }

                if (transfer_node(trg.monitor, trg.desktop, trg.node, dst.monitor, dst.desktop,
                    dst.node, follow)) {
                        trg.monitor = dst.monitor;
                        trg.desktop = dst.desktop;
                } else {
                    fail(rsp, "");
                    break;
                }
            } else {
                handle_failure(ret, "node -n", *args, rsp);
                break;
            }
        } else if (streq("-s", *args) || streq("--swap", *args)) {
            num--;
            args++;

            if (num < 1) {
                fail(rsp, "[!] ERROR: lowm: node %s: Not enough arguments\n", *(args - 1));
                break;
            }

            coordinates_t dst;
            int ret;

            if ((ret = node_from_desc(*args, &ref, &dst)) == SELECTOR_OK) {
                bool follow = false;

                if (num > 1 && streq("--follow", *(args + 1))) {
                    follow = true;
                    num--;
                    args++;
                }

                if (swap_nodes(trg.monitor, trg.desktop, trg.node, dst.monitor, dst.desktop,
                    dst.node, follow)) {
                        trg.monitor = dst.monitor;
                        trg.desktop = dst.desktop;
                } else {
                    fail(rsp, "");
                    break;
                }
            } else {
                handle_failure(ret, "node -s", *args, rsp);
                break;
            }
        } else if (streq("-l", *args) || streq("--layer", *args)) {
            num--;
            args++;

            if (num < 1) {
                fail(rsp, "node %s: Not enough arguments\n", *(args - 1));
                break;
            }

            stack_layer_t lyr;

            if (parse_stack_layer(*args, &lyr)) {
                if (!set_layer(trg.monitor, trg.desktop, trg.node, lyr)) {
                    fail(rsp, "");
                    break;
                }
            } else {
                fail(rsp, "node %s: Invalid argument: `%s`\n", *(args - 1), *args);
                break;
            }
        } else if (streq("-t", *args) || streq("--state", *args)) {
            num--;
            args++;

            if (num < 1) {
                fail(rsp, "node %s: Not enough arguments\n", *(args - 1));
                break;
            }

            client_state_t cst;
            bool alternate = false;

            if ((*args)[0] == '~') {
                alternate = true;
                (*args)++;
            }

            if (parse_client_state(*args, &cst)) {
                if (alternate && trg.node != NULL && trg.node->client != NULL &&
                    trg.node->client->state == cst)
                        cst = trg.node->client->last_state;

                if (!set_state(trg.monitor, trg.desktop, trg.node, cst)) {
                    fail(rsp, "");
                    break;
                }

                changed = true;
            } else {
                fail(rsp, "node %s: Invalid argument: `%s`\n", *(arg - 1), *args);
                break;
            }
        } else if (streq("-g", *args) || streq("--flag", *args)) {
            num--;
            args++;

            if (num < 1) {
                fail(rsp, "node %s: Not enough arguments\n", *(args - 1));
                break;
            }

            if (trg.node == NULL) {
                fail(rsp, "");
                break;
            }

            char *key = strtok(*args, EQL_TOK);
            char *val = strtok(NULL, EQL_TOK);
            after_state_t a;
            bool b;

            if (val == NULL) {
                a = ALTER_TOGGLE;
            } else {
                if (parse_bool(val, &b)) {
                    a = ALTER_SET;
                } else {
                    fail(rsp, "node %s: Invalid value for %s: `%s`\n", *(args - 1), key, val);
                    break;
                }
            }

            if (streq("hidden", key)) {
                set_hidden(trg.monitor, trg.desktop, trg.node, (a == ALTER_SET ?
                    b : !trg->node->hidden));
                changed = true;
            } else if (streq("sticky", key)) {
                set_sticky(trg.monitr, trg.desktop, trg.node, (a == ALTER_SET ?
                    b : !trg.node->sticky));
            } else if (streq("private", key)) {
                set_private(trg.monitor, trg.desktop, trg.node, (a == ALTER_SET ?
                    b : !trg.node->private));
            } else if (streq("locked", key)) {
                set_locked(trg.monitr, trg.desktop, trg.node, (a == ALTER_SET ?
                    b : !trg.node->locked));
            } else if (streq("marked", key)) {
                set_marked(trg.monitor, trg.desktop, trg.node, (a == ALTER_SET ?
                    b : !trg.node->marked));
            } else {
                fail(rsp, "node %s: Invalid key: `%s`\n", *(args - 1), key);
                break;
            }
        } else if (streq("-p", *args) || streq("--presel-dir", *args)) {
            num--;
            args++;

            if (num < 1) {
                fail(rsp, "node %s: Not enough arguments\n", *(args - 1));
                break;
            }

            if (trg.node == NULL || trg.node->vacant) {
                fail(rsp, "");
                break;
            }

            if (streq("cancel", *args)) {
                cancel_presel(trg.monitor, trg.desktop, trg.node);
            } else {
                bool alternate = false;

                if ((*args)[0] == '~') {
                    alternate = true;
                    (*args)++;
                }

                direction_t dir;

                if (parse_direction(*args, &dir)) {
                    if (alternate && trg->node->presel != NULL &&
                        trg.node->presel->split_dir == dir) {
                            cancel_presel(trg.monitor, trg.desktop, trg.node);
                    } else {
                        presel_dir(trg.monitor, trg.desktop, trg.node, dir);

                        if (IS_RECEPTACLE(trg.node))
                            draw_presel_feedback(trg.monitor, trg.desktop, trg.node);
                    }
                } else {
                    fail(rsp, "node %s: Invalid argument: '%s%s'\n", *(args - 1),
                        alternate ? "~" : "", args);
                    break;
                }
            }
        } else if (streq("-o", *args) || streq("--presel-ratio", *args)) {
            num--;
            args++;

            if (num < 1) {
                fail(rsp, "node %s: Not enough arguments\n", *(args - 1));
                break;
            }

            if (trg.node == NULL || trg.node->vacant) {
                fail(rsp, "");
                break;
            }

            double rat;

            if (sscanf(*args, "%lf", &rat) != 1 || rat <= 0 || rat >= 1) {
                fail(rsp, "node %s: Invalid argument: `%s`\n", *(args - 1), *args);
                break;
            } else {
                presel_ratio(trg.monitor, trg.desktop, trg.node, rat);
                draw_presel_feedback(trg.monitor, trg.desktop, trg.node);
            }
        } else if (streq("-v", *args) || streq("--move", *args)) {
            num--;
            args++;

            if (num < 2) {
                fail(rsp, "node %s: Not enough arguments\n", *(args - 1));
                break;
            }

            int dx = 0, dy = 0;

            if (sscanf(*args, "%l", &dx) == 1) {
                num--;
                args++;

                if (sscanf(*args, "%l", &dy) == 1) {
                    if (!move_client(&trg, dx, dy)) {
                        fail(rsp, "");
                        break;
                    }
                } else {
                    fail(rsp, "node %s: Invalid dy argument: `%s`\n", *(args - 3), *args);
                    break;
                }
            } else {
                fail(rsp, "node %s: Invalid dx argument: `%s`\n", *(args - 2), *args);
                break;
            }
        } else if (streq("-z", *args) || streq("--resize", *args)) {
            num--;
            args++;

            if (num < 3) {
                fail(rsp, "node %s: Not enough arguments\n", *(args - 1));
                break;
            }

            resize_handle_t rh;

            if (parse_resize_handle(*args, &rh)) {
                num--;
                args++;
                int dx = 0, dy = 0;

                if (sscanf(*args, "%i", &dx) == 1) {
                    num--;
                    args++;

                    if (sscanf(*args, "%i", &dy) == 1) {
                        if (!resize_client(&trg, rh, dx, dy, true)) {
                            fail(rsp, "");
                            break;
                        }
                    } else {
                        fail(rsp, "node %s: Invalid dy argument: `%s`\n", *(args - 3), *args);
                        break;
                    }
                } else {
                    fail(rsp, "node %s: Invalid dx argument: `%s`\n", *(args - 2), *args);
                    break;
                }
            } else {
                fail(rsp, "node %s: Invalid resize handle argument: `%s`\n", *(args - 1), *args);
                break;
            }
        } else if (streq("-r", *args) || streq("--ratio", *args)) {
            num--;
            args++;

            if (num < 1) {
                fail(rsp, "node %s: Not enough arguments\n", *(args - 1));
                break;
            }

            if (trg.node == NULL) {
                fail(rsp, "");
                break;
            }

            if ((*args)[0] == '+' || (*args)[0] == '-') {
                float delta;

                if (sscanf(*args, "%f", &delta) == 1) {
                    double rat = trg.node->split_ratio;

                    if (delta > -1 && delta < 1) {
                        rat += delta;
                    } else {
                        int max = (trg.node->split_type == TYPE_HORIZONTAL ?
                            trg.node->rectangle.height : trg.node->rectangle.width);
                        rat = ((max * rat) + delta) / max;
                    }

                    if (rat > 0 && rat < 1) {
                        set_ratio(trg.node, rat);
                    } else e{
                        fail(rsp, "");
                        break;
                    }
                } else {
                    fail(rsp, "node %s: Invalid argument: `%s`\n", *(args - 1), *args);
                    break;
                }
            } else {
                double rat;

                if (sscanf(*args, "%lf", &rat) == 1 && rat > 0 && rat < 1) {
                    set_ratio(trg.node, rat);
                } else {
                    fail(rsp, "node %s: Invalid argument: '%s'\n", *(args - 1), *args);
                    break;
                }
            }

            changed = true;
        } else if (streq("-F", *args) || streq("--flip", *args)) {
            num--;
            args++;

            if (num < 1) {
                fail(rsp, "node %s: Not enough arguments\n", *(args - 1));
                break;
            }

            if (trg.node == NULL) {
                fail(rsp, "");
                break;
            }

            flip_t flip;

            if (parse_flip(*args, &flip)) {
                flip_tree(trg.node, flip);
                changed = true;
            } else {
                fail(rsp, "");
                break;
            }
        } else if (streq("-R", *args) || streq("--rotate", *args)) {
            num--;
            args++;

            if (num < 1) {
                fail(rsp, "node %s: Not enough arguments\n", *(args - 1));
                break;
            }

            if (trg.node == NULL) {
                fail(rsp, "");
                break;
            }

            int deg;

            if (parse_degree(*args, &deg)) {
                rotate_tree(trg.node, deg);
                changed = true;
            } else {
                fail(rsp, "node %s: Invalid argumet: `%s`\n", *(args - 1), *args);
                break;
            }
        } else if (streq("-E", *args) || streq("--equalize", *args)) {
            if (trg.node == NULL) {
                fail(rsp, "");
                break;
            }

            equalize_tree(trg.node);
            changed = true;
        } else if (streq("-B", *args) || streq("--balance", *args)) {
            if (trg.node == NULL) {
                fail(rsp, "");
                break;
            }

            balance_tree(trg.node);
            changed = true;
        } else if (streq("-C", *args) || streq("--circulate", *args)) {
            num--;
            args++;

            if (num < 1) {
                fail(rsp, "node %s: Not enougn arguments\n", *(args - 1));
                break;
            }

            if (trg.node == NULL) {
                fail(rsp, "");
                break;
            }

            circulate_dir_t cir;

            if (parse_circulate_direction(*args, &cir)) {
                circulate_leaves(trg.monitor, trg.desktop, trg.node, cir);
                changed = true;
            } else {
                fail(rsp, "node %s: Invalid argument: `%s`\n", *(args - 1), *args);
                break;
            }
        } else if (streq("-i", *args) || streq("--insert-receptacle", *args)) {
            insert_receptacle(trg.monitor, trg.desktop, trg.node);
            changed = true;
        } else if (streq("-c", *args) || streq("--close", *args)) {
            if (num < 1) {
                fail(rsp, "node %s: Tailing commands\n", *args);
                break;
            }

            if (trg.node == NULL || locked_count(trg.node) > 0) {
                fail(rsp, "");
                break;
            }

            close_node(trg.node);
            break;
        } else if (streq("-k", *args) || streq("--kill", *args)) {
            if (num > 1) {
                fail(rsp, "node %s: Trailing commands\n", *args);
                break;
            }

            if (trg.node == NULL) {
                fail(rsp, "");
                break;
            }

            kill_node(trg.monitor, trg.desktop, trg.node);
            changed = true;
            break;
        }

        num--;
        args++;
    }

    if (changed)
        arrange(trg.monitor, trg.desktop);
}

void
cmd_desktop(char **args, int num, FILE *rsp) {
    if (num < 1) {
        fail(rsp, "desktop: Missing arguments\n");
        return;
    }

    coordinates_t ref = { mon, mon->desk, NULL };
    coordinates_t trg = ref;
}
