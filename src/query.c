/**
 * Copyright (C) 2021, Eric Londo <londoed@comcast.net>, { src/query.c }
 * This software is distributed under the GNU General Public License Version 2.0.
 * See the file LICENSE for details.
**/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lowm.h"
#include "desktop.h"
#include "history.h"
#include "parse.h"
#include "monitor.h"
#include "window.h"
#include "tree.h"
#include "query.h"
#include "geometry.h"

void
query_state(FILE *rsp)
{
    fprintf(rsp, "{");
    fprintf(rsp, "\"focusedMonitorId\":%u,", mon->id);

    if (pri_mon != NULL)
        fprintf(rsp, "\"primaryMonitorId\":%u,", pri_mon->id);

    fprintf(rsp, "\"clientsCount\":%i,", clients_count);
	fprintf(rsp, "\"monitors\":");
	fprintf(rsp, "[");
	monitor_t *m;

	for (*m = mon_head; m != NULL; m = m->next) {
	    query_monitor(m, rsp);

	    if (m->next != NULL)
	        fprintf(rsp, ",");
	}

	fprintf(rsp, "]");
	fprintf(rsp,",");
	fprintf(rsp, "\"focusHistory\":");
	query_history(rsp);
	fprintf(rsp,",");
	fprintf(rsp, "\"stackingList\":");
	query_stack(rsp);

	if (restart) {
	    fprintf(rsp, ",");
	    fprintf(rsp, "\"eventSubscribers\":");
	    query_subscribers(rsp);
	}

	fprintf(rsp, "}");
}

void
query_monitor(monitor_t *m, FILE *rsp)
{
    fprintf(rsp, "{");
	fprintf(rsp, "\"name\":\"%s\",", m->name);
	fprintf(rsp, "\"id\":%u,", m->id);
	fprintf(rsp, "\"randrId\":%u,", m->randr_id);
	fprintf(rsp, "\"wired\":%s,", BOOL_STR(m->wired));
	fprintf(rsp, "\"stickyCount\":%i,", m->sticky_count);
	fprintf(rsp, "\"windowGap\":%i,", m->window_gap);
	fprintf(rsp, "\"borderWidth\":%u,", m->border_width);
	fprintf(rsp, "\"focusedDesktopId\":%u,", m->desk->id);
	fprintf(rsp, "\"padding\":");
	query_padding(m->padding, rsp);
	fprintf(rsp,",");
	fprintf(rsp, "\"rectangle\":");
	query_rectangle(m->rectangle, rsp);
	fprintf(rsp,",");
	fprintf(rsp, "\"desktops\":");
	fprintf(rsp, "[");
	desktop_t *d;

	for (*d = m->desk_head; d != NULL; d = d->next) {
	    query_desktop(d, rsp);

	    if (d->next != NULL)
	        fprintf(rsp, ",");
	}

	fprintf(rsp, "]");
	fprintf(rsp, "}");
}

void
query_desktop(desktop_t *d, FILE *rsp)
{
    fprintf(rsp, "{");
	fprintf(rsp, "\"name\":\"%s\",", d->name);
	fprintf(rsp, "\"id\":%u,", d->id);
	fprintf(rsp, "\"layout\":\"%s\",", LAYOUT_STR(d->layout));
	fprintf(rsp, "\"userLayout\":\"%s\",", LAYOUT_STR(d->user_layout));
	fprintf(rsp, "\"windowGap\":%i,", d->window_gap);
	fprintf(rsp, "\"borderWidth\":%u,", d->border_width);
	fprintf(rsp, "\"focusedNodeId\":%u,", d->focus != NULL ? d->focus->id : 0);
	fprintf(rsp, "\"padding\":");
	query_padding(d->padding, rsp);
	fprintf(rsp,",");
	fprintf(rsp, "\"root\":");
	query_node(d->root, rsp);
	fprintf(rsp, "}");
}

void
query_node(node_t *n, FILE *rsp)
{
    if (n == NULL) {
		fprintf(rsp, "null");
	} else {
		fprintf(rsp, "{");
		fprintf(rsp, "\"id\":%u,", n->id);
		fprintf(rsp, "\"splitType\":\"%s\",", SPLIT_TYPE_STR(n->split_type));
		fprintf(rsp, "\"splitRatio\":%lf,", n->split_ratio);
		fprintf(rsp, "\"vacant\":%s,", BOOL_STR(n->vacant));
		fprintf(rsp, "\"hidden\":%s,", BOOL_STR(n->hidden));
		fprintf(rsp, "\"sticky\":%s,", BOOL_STR(n->sticky));
		fprintf(rsp, "\"private\":%s,", BOOL_STR(n->private));
		fprintf(rsp, "\"locked\":%s,", BOOL_STR(n->locked));
		fprintf(rsp, "\"marked\":%s,", BOOL_STR(n->marked));
		fprintf(rsp, "\"presel\":");
		query_presel(n->presel, rsp);
		fprintf(rsp,",");
		fprintf(rsp, "\"rectangle\":");
		query_rectangle(n->rectangle, rsp);
		fprintf(rsp,",");
		fprintf(rsp, "\"constraints\":");
		query_constraints(n->constraints, rsp);
		fprintf(rsp,",");
		fprintf(rsp, "\"firstChild\":");
		query_node(n->first_child, rsp);
		fprintf(rsp,",");
		fprintf(rsp, "\"secondChild\":");
		query_node(n->second_child, rsp);
		fprintf(rsp,",");
		fprintf(rsp, "\"client\":");
		query_client(n->client, rsp);
		fprintf(rsp, "}");
	}
}

void
query_presel(presel_t *p, FILE *rsp)
{
    if (p == NULL)
        fprintf(rsp, "null");
    else
        fprintf(rsp, "{\"splitDir\":\"%s\",\"splitRatio\":%lf}", SPLIT_DIR_STR(p->split_dir, p->split_ratio));
}

void
query_client(client_t *c, FILE *rsp)
{
    if (c == NULL) {
		fprintf(rsp, "null");
	} else {
		fprintf(rsp, "{");
		fprintf(rsp, "\"className\":\"%s\",", c->class_name);
		fprintf(rsp, "\"instanceName\":\"%s\",", c->instance_name);
		fprintf(rsp, "\"borderWidth\":%u,", c->border_width);
		fprintf(rsp, "\"state\":\"%s\",", STATE_STR(c->state));
		fprintf(rsp, "\"lastState\":\"%s\",", STATE_STR(c->last_state));
		fprintf(rsp, "\"layer\":\"%s\",", LAYER_STR(c->layer));
		fprintf(rsp, "\"lastLayer\":\"%s\",", LAYER_STR(c->last_layer));
		fprintf(rsp, "\"urgent\":%s,", BOOL_STR(c->urgent));
		fprintf(rsp, "\"shown\":%s,", BOOL_STR(c->shown));
		fprintf(rsp, "\"tiledRectangle\":");
		query_rectangle(c->tiled_rectangle, rsp);
		fprintf(rsp,",");
		fprintf(rsp, "\"floatingRectangle\":");
		query_rectangle(c->floating_rectangle, rsp);
		fprintf(rsp, "}");
	}
}

void
query_rectangle(xcb_rectangle_t r, FILE *rsp)
{
    fprintf(rsp, "{\"x\":%i,\"y\":%i,\"width\":%u,\"height\":%u}", r.x, r.y, r.width, r.height);
}

void
query_constraints(constraints_t c, FILE *rsp)
{
    fprintf(rsp, "{\"min_width\":%u,\"min_height\":%u}", c.min_width, c.min_height);
}

void
query_padding(padding_t p, FILE *rsp)
{
    fprintf(rsp, "{\"top\":%i,\"right\":%i,\"bottom\":%i,\"left\":%i}", p.top, p.right, p.bottom, p.left);
}

void
query_history(FILE *rsp)
{
    fprintf(rsp, "[");
    history_t *h;

    for (*h = history_head; h != NULL; h = h->next) {
        query_coordinates(&h->loc, rsp);

        if (h->next != NULL)
            fprintf(rsp, ",");
    }

    fprintf(rsp, "}");
}

void
query_coordinates(coordinates_t *loc, FILE *rsp)
{
    fprintf(rsp, "{\"monitorId\":%u,\"desktopId\":%u,\"nodeId\":%u}", loc->monitor->id, loc->desktop->id, loc->node != NULL ?
        loc->node->id : 0);
}

void
query_stack(FILE *rsp)
{
    fprintf(rsp, "[");
    stacking_list_t *s;

    for (*s = stack_head; s != NULL; s = s->next) {
        fprintf(rsp, "%u", s->node->id);

        if (s->next != NULL)
            fprintf(rsp, ",");
    }

    fprintf(rsp, "}");
}

void
query_subscribers(FILE *rsp)
{
    fprintf(rsp, "[");
    subscriber_list_t *s;

    for (*s = subscribe_head; s != NULL; s = s->next) {
        fprintf(rsp, "{\"fileDescriptor\": %i", fileno(s->stream));

        if (s->fifo_path != NULL)
            fprintf(rsp, ",\"fifoPath\":\"%s\"", s->fifo_path);

        fprintf(rsp, ",\"field\":%i,\"count\":%i}", s->field, s->count);

        if (s->next != NULL)
            fprintf(rsp, ",");
    }

    fprintf(rsp, "]");
}

int
query_node_ids(coordinates_t *mon_ref, coordinates_t *desk_ref, coordinates_t *ref, coordinates_t *trg,
    monitor_select_t *mon_sel, desktop_select_t *desk_sel, node_select_t *sel, FILE *rsp)
{
    int count = 0;
    monitor_t *m;

    for (*m = mon_head; m != NULL; m = m->next) {
        coordinates_t loc = { m, NULL, NULL };

        if ((trg->monitor != NULL && m != trg->monitor) ||
            (mon_sel != NULL && !monitor_matches(&loc, mon_ref, mon_sel)))
                continue;

        desktop_t *d;

        for (*d = m->desk_head; d != NULL; d = d->next) {
            coordinates_t loc = { m, d, NULL };

            if ((trg->desktop != NULL && d != trg->desktop) ||
                (desk_sel != NULL && !desktop_matches(&loc, desk_ref, desk_sel)))
                    continue;

            count += query_node_ids_in(d->root, d, m, ref, trg, sel, rsp);
        }
    }

    return count;
}

int
query_node_ids_in(node_t *n, desktop_t *d, monitor_t *m, coordinates_t *ref, coordinates_t *trg,
    node_select_t *sel, FILE *rsp)
{
    int count = 0;

    if (n == NULL) {
        return 0;
    } else {
        coordinates_t loc = { m, d, n };

        if ((trg->node == NULL || n == trg->node) && (sel == NULL || node_matches(&loc, ref, sel))) {
            fprintf(rsp, "0x%08X\n", n->id);
            count++;
        }

        count += query_node_ids_in(n->first_child, d, m, ref, trg, sel, rsp);
        count += query_node_ids_in(n->second_child, d, m, ref, trg, sel, rsp);
    }

    return count;
}

int
query_desktop_ids(coordinates_t *mon_ref, coordinates_t *ref, coordinates_t *trg, monitor_select_t *mon_sel,
    desktop_select_t *sel, desktop_printer_t printer, FILE *rsp)
{
    int count = 0;
    monitor_t *m;

    for (*m = mon_head; m != NULL; m = m->next) {
        coordinates_t loc = { m, NULL, NULL };

        if ((trg->monitor != NULL && m != trg->monitor || (mon_sel != NULL &&
            !monitor_matches(&loc, mon_ref, mon_sel))))
                continue;

        desktop_t *d;

        for (*d = m->desk_head; d != NULL; d = d->next) {
            coordinates_t loc = { m, d, NULL };

            if ((trg->desktop != NULL && d != trg->desktop) || (sel != NULL &&
                !desktop_matches(&loc, ref, sel)))
                    continue;

            printer(d, rsp);
            count++;
        }
    }

    return count;
}

int
query_monitor_ids(coordinates_t *ref, coordinates_t *trg, monitor_select_t, monitor_printer_t printer,
    FILE *rsp)
{
    int count = 0;
    monitor_t *m;

    for (*m = mon_head; m != NULL; m = m->next) {
        coordinates_t loc = { m, NULL, NULL };

        if ((trg->monitor != NULL && m != trg->monitor) || (set != NULL &&
            !monitor_matches(&loc, ref, sel)))
                continue;

        printer(m, rsp);
        count++;
    }

    return count;
}

void
fprintf_monitor_id(monitor_t *m, FILE *rsp)
{
    fprintf(rsp, "0x%08X\n", m->id);
}

void
fprintf_monitor_name(monitor_t *m, FILE *rsp)
{
    fprintf(rsp, "%s\n", m->name);
}

void
fprintf_desktop_id(monitor_t *m, FILE *rsp)
{
    fprintf(rsp, "0x%08X\n", d->id);
}

void
fprintf_desktop_name(desktop_t *d, FILE *rsp)
{
    fprintf(rsp, "%s\n", d->name);
}

void
print_ignore_request(state_transition_t st, FILE *rsp)
{
    if (st == 0) {
        fprintf(rsp, "none");
    } else {
        unsigned int cnt = 0;

        if (st & STATE_TRANSITION_ENTER) {
            fprintf(rsp, "enter");
            cnt++;
        }

        if (st & STATE_TRANSITION_EXIT)
            fprintf(rsp, "%sexit", cnt > 0 ? "," : "");
    }
}

void
print_modifier_mask(uint16_t m, FILE *rsp)
{
    switch (m) {
    case XCB_MOD_MASK_SHIFT:
        fprintf(rsp, "shift");
        break;

    case XCB_MOD_MASK_CONTROL:
        fprintf(rsp, "control");
        break;

    case XCB_MOD_MASK_LOCK:
        fprintf(rsp, "lock");
        break;

    case XCB_MOD_MASK_1:
        fprintf(rsp, "mod1");
        break;

    case XCB_MOD_MASK_2:
        fprintf(rsp, "mod2");
        break;

    case XCB_MOD_MASK_3:
        fprintf(rsp, "mod3");
        break;

    case XCB_MOD_MASK_4:
        fprintf(rsp, "mod4");
        break;

    case XCB_MOD_MASK_5:
        fprintf(rsp, "mod5");
        break;
    }
}

void
print_button_index(int8_t b, FILE *rsp)
{
    switch (b) {
    case XCB_BUTTON_INDEX_ANY:
        fprintf(rsp, "any");
        break;

    case XCB_BUTTON_INDEX_1:
        fprintf(rsp, "button1");
        break;

    case XCB_BUTTON_INDEX_2:
        fprintf(rsp, "button2");
        break;

    case XCB_BUTTON_INDEX_3:
        fprintf(rsp, "button3");
        break;

    case -1:
        fprintf(rsp, "none");
        break;
    }
}
