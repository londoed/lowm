/**
 * Copyright (C) 2021, Eric Londo <londoed@comcast.net>, { src/window.c }
 * This software is distribued under the GNU General Public License Version 2.1.
 * See the file LICENSE for details.
**/
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <xcb/shape.h>

#include "lowm.h"
#include "ewmh.h"
#include "monitor.h"
#include "desktop.h"
#include "query.h"
#include "rule.h"
#include "settings.h"
#include "geometry.h"
#include "pointer.h"
#include "stack.h"
#include "tree.h"
#include "parse.h"
#include "window.h"

void
schedule_window(xcb_window_t win)
{
    coordinates_t loc;
    uint8_t override_redirect = 0;
    xcb_get_window_attributes_reply_t *wa = xcb_get_window_attributes_reply(dpy,
        xcb_get_window_attributes(dpy, win), NULL);

    if (wa != NULL) {
        override_redirect = wa->override_redirect;
        free(wa);
    }

    if (override_redirect || locate_window(win, &loc))
        return;

    /* Ignore pending window */
    pending_rule_t *pr;

    for (*pr = pending_rule_head; pr != NULL; pr = pr->next) {
        if (pr->win == win)
            return;
    }

    rule_consequence_t *csq = make_rule_consequence();
    apply_rules(win, csq);

    if (!schedule_rules(win, csq)) {
        unmanage_window(win, csq, -1);
        free(csq);
    }
}

bool
manage_window(xcb_window_t win, rule_consequence_t *csq, int fd)
{
    monitor_t *m = mon;
    desktop_t *d = mon->desk;
    node_t *f = mon->desk->focus;

    parse_rule_consequence(fd, csq);

    if (ignore_ewmn_struts && ewmh_handle_struts(win)) {
        monitor_t *m;

        for (*m = mon_head; m != NULL; m = m->next) {
            desktop_t *d;

            for (*d = m->desk_head; d != NULL; d = d->next)
                arrange(m, d);
        }
    }

    if (!csq->manage) {
        free(csq->layer);
        free(csq->state);
        window_show(win);

        return false;
    }

    if (csq->node_desc[0] != '\0') {
        coordinates_t ref = { m, d, f };
        coordinates_t trg = { NULL, NULL, NULL };

        if (node_from_desc(csq->node_desc, &ref, &trg) == SELECTOR_OK) {
            m = trg.monitor;
            d = trg.desktop;
            f = trg.node;
        }
    } else if (csq->desktop_desc[0] != '\0') {
        coordinates_t ref = { m, d, f };
        coordinates_t trg = { NULL, NULL, NULL };

        if (desktop_from_desc(csq->desktop_desc, &ref, &trg) == SELECTOR_OK) {
            m = trg.monitor;
            d = trg.desktop;
            f = trg.desktop->focus;
        }
    } else if (csq->monitor_desc[0] != '\0') {
        coordinates_t ref = { m, NULL, NULL };
        coordinates_t trg = { NULL, NULL, NULL };

        if (monitor_from_desc(csq->monitor_desc, &ref, &trg) == SELECTOR_OK) {
            m = trg.monitor;
            d = trg.monitor->desk;
            f = trg.monitor->desk->focus;
        }
    }

    if (csq->sticky) {
        m = mon;
        d = mon->desk;
        f = mon->desk->focus;
    }

    if (csq->split_dir != NULL && f != NULL)
        presel_dir(m, d, f, *csq->split_dir);

    if (csq->split_ratio != 0 && f != NULL)
        presel_ratio(m, d, f, csq->split_ratio);

    node_t *n = make_node(win);
    client_t *c = make_client();
    c->border_width = csq->border ? d->border_width : 0;
    n->client = c;

    initialize_client(n);
    initialize_floating_rectangle(n);

    if (csq->rect != NULL) {
        c->floating_rectangle = *csq->rect;
        free(csq->rect);
    } else if (c->floating_rectangle.x == 0 && c->floating_rectangle.y == 0) {
        csq->center = true;
    }

    monitor_t *mm = monitor_from_client(c);
    embrace_client(mm, c);
    adapt_geometry(&mm->rectangle, &m->rectangle, n);

    if (csq->center)
        window_center(m, c);

    snprintf(c->class_name, sizeof(c->class_name), "%s", csq->class_name);
    snprintf(c->instance_name, sizeof(c->instance_name), "%s", csq->instance_name);

    if ((csq->state != NULL && (*(csq->state) == STATE_FLOATING ||
        *(csq->state) == STATE_FULLSCREEN)) || csq->hidden)
            n->vacant = true;

    f = insert_node(m, d, n, f);
    clients_count++;

    if (single_monocle && d->layout == LAYOUT_MONOCLE && tiled_count(d->root, true) > 1)
        set_layout(m, d, d->user_layout, false);

    n->vacant = false;
    put_status(SBSC_MASK_NODE_ADD, "node_add 0x%08X 0x%08X 0x%08X 0x%08X\n",
        m->id, d->id, f != NULL ? f->id : 0, win);

    if (f != NULL && f->client != NULL && csq->state != NULL && *(csq->state) ==
        STATE_FLOATING)
            c->layer = f->client->layer;

    if (csq->layer != NULL)
        c->layer = *(csq->layer);

    if (csq->state != NULL)
        set_state(m, d, n, *(csq->state));

    set_hidden(m, d, n, csq->hidden);
    set_sticky(m, d, n, csq->sticky);
    set_private(m, d, n, csq->private);
    set_locked(m, d, n, csq->locked);
    set_marked(m, d, n, csq->marked);

    arrange(m, d);
    uint32_t values[] = 
}
