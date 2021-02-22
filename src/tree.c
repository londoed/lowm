/**
 * Copyright (C) 2021, Eric Londo <londoed@comcast.net>, { src/tree.c }
 * This software is distributed under the GNU General Public License 2.0.
 * See the file LICENSE for details.
**/
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <limits.h>

#include "lowm.h"
#include "desktop.h"
#include "ewmh.h"
#include "history.h"
#include "monitor.h"
#include "query.h"
#include "geometry.h"
#include "subscribe.h"
#include "settings.h"
#include "pointer.h"
#include "stack.h"
#include "window.h"
#include "tree.h"

void
arrange(monitor_t *m, desktop_t *d)
{
    if (d->root == NULL)
        return;

    xcb_rectangle_t rect = m->rectangle;

    rect.x += m->padding.left + d->padding.left;
    rect.y += m->padding.top + d->padding.top;
    rect.width -= m->padding.left + d->padding.left + d->padding.right + m->padding.right;
    rect.height -= m->padding.top + d->padding.top + d->padding.bottom + m->padding.bottom;

    if (d->layout == LAYOUT_MONOCLE) {
        rect.x += monocle_padding.left;
        rect.y += monocle_padding.top;
        rect.width -= monocle_padding.left + monocle_padding.right;
        rect.height -= monocle_padding.top + monocle_padding.bottom;
    }

    if (!gapless_monocle || d->layout != LAYOUT_MONOCLE) {
        rect.x += d->window_gap;
        rect.y += d->window_gap;
        rect.width -= d->window_gap;
        rect.height -= d->window_gap;
    }

    apply_layout(m, d, d->root, rect, rect);
}

void
apply_layout(monitor_t *m, desktop_t *d, node_t *n, xcb_rectangle_t rect, xcb_rectangle_t root_rect)
{
    if (n == NULL)
        return;

    n->rectangle = rect;

    if (n->presel != NULL)
        draw_presel_feedback(m, d, n);

    if (is_leaf(n)) {
        if (n->client == NULL)
            return;

        unsigned int bw;
        bool the_only_window = !m->prev && !m->next && d->root->client;

        if ((borderless_monocle && data->layout == LAYOUT_MONOCLE && IS_TILED(n->client)) ||
            (borderless_singleton && the_only_window) ||
            n->client->state == STATE_FULLSCREEN)
                bw = 0;
        else
                bw = n->client->border_width;

        xcb_rectangle_t r, xcb_rectangle_t = get_window_rectangle(n);

        /* Tiled and pseudo-tiled client */
        if (s == STATE_TILED || s == STATE_PSEUDO_TILED) {
            int wg = (gapless_monocle && d->layout == LAYOUT_MONOCLE ? 0 : d->window_gap);
            r = rect;
            int bleed = wg + 2 * bw;

            r.width = (bleed < r.width ? r.width - bleed : 1);
            r.height = (bleed < r.height ? r.height - bleed : 1);

            if (s == STATE_PSEUDO_TILED) {
                xcb_rectangle_t f = n->client->floating_rectangle;
                r.width = MIN(r.width, f.width);
                r.height = MIN(r.height, f.height);

                if (center_pseudo_tiled) {
                    r.x = rect.x - bw + (rect.width - wg - r.width) / 2;
                    r.y = rect.y - bw + (rect.height - wg - r.height) / 2;
                }
            }

            n->client->tiled_rectangle = r;
        } else if (s == STATE_FLOATING) {
            r = n->client->floating_rectangle;

        /* Fullscreen client */
        } else {
            r = m->rectangle;
            n->client->tiled_rectangle = r;
        }

        apply_size_hints(n->client, &r.width, &r.height);

        if (!rect_eq(r, cr)) {
            window_move_resize(n->id, r.x, r.y, r.width, r.height);

            if (!grabbing)
                put_status(SBSC_MASK_NODE_GEOMETRY, "node_geometry 0x%08X 0x%08X 0x%08X "
                    "%ux%u+%i+%i\n", m->id, d->id, n->id, r.width, r.height, r.x, r.y);
        }

        window_border_width(n->id, bw);
    } else {
        xcb_rectangle_t first_rect;
        xcb_rectangle_t second_rect;

        if (d->layout == LAYOUT_MONOCLE || n->first_child->vacant || n->second_child->vacant) {
            first_rect = second_rect = rect;
        } else {
            unsigned int fence;

            if (n->split_type == TYPE_VERTICAL) {
                fence = rect.width * n->split_ratio;

                if ((n->first_child->constraints.min_width + n->second_child->constraints.min_width) <=
                    rect.width) {
                    if (fence < n->first_child->contsraints.min_width) {
                        fence = n->first_child->contstraints.min_width;
                        n->split_ratio = (double)fence / (double)rect.width;
                    } else if (fence > (uint16_t)(rect.width - n->second_child->constraints.min_width)) {
                        fence = (rect.width - n->second_child->constraints.min_width);
                        n->split_ration = (double)fence / (double)rect.width;
                    }
                }

                first_rect = (xcb_rectangle_t) { rect.x, rect.y, fence, rect.height };
                second_rect = (xcb_rectangle_t) { rect.x + fence, rect.y, rect.width - fence, rect.height };
            } else {
                fence = rect.height * n->split_ratio;

                if ((n->first_child->constraints.min_height + n->second_child->constraints.min_height) <=
                    rect.height) {
                    if (fence < n->first_child->constraints.min_height) {
                        fence = n->first_child.constraints.min_height;
                        n->split_ratio = (double)fence / (double)rect.height;
                    } else if (fence > (uint16_t)(rect.height - n->second_child->constraints.min_height)) {
                        fence = (rect.height - n->second_child->constraints.min_height);
                        n->split_ratio = (double)fence / (double)rect.height;
                    }
                }

                first_rect = (xcb_rectangle_t) { rect.x, rect.y, rect.width, fence };
                second_rect = (xcb_rectangle_t) { rect.x, rect.y + fence, rect.width, rect.height - fence};
            }
        }

        apply_layout(m, d, n->first_child, first_rect, root_rect);
        apply_layout(m, d, n->second_child, second_rect, root_rect);
    }
}

presel_t *
make_presel(void)
{
    presel_t *p = calloc(1, sizeof(presel_t));

    p->split_dir = DIR_EAST;
    p->split_ration = split_ratio;
    p->feedback = XCB_NONE;

    return p;
}

void
set_ratio(node_t *n, double rat)
{
    if (n == NULL)
        return;

    n->split_ratio = rat;
}

void
presel_dir(monitor_t *m,, desktop_t *d, node_t *n, direction_t dir)
{
    if (n->presel == NULL)
        n->presel = make_presel();

    n->presel->split_dir = dir;
    put_status(SBSC_MASK_NODE_PRESEL, "node_presel 0x%08X 0x%08X 0x%08X dir %s\n", m->id, d->id,
        n->id, SPLIT_DIR_STR(dir));
}

void
presel_ratio(monitor_t *m, desktop_t *d, node_t *n, double ratio)
{
    if (n->presel == NULL)
        n->presel = make_presel();

    n->presel->split_ratio = ratio;
    put_status(SBSC_MASK_NODE_PRESEL, "node_presel 0x%08X 0x%08X ratio %lf\n", m->id, d->id,
        n->id, ratio);
}

void
cancel_preset(monitor_t *m, desktop_t *d, node_t *n)
{
    if (n->preset == NULL)
        return;

    if (n->presel->feedback != XCB_NONE)
        xcb_destroy_window(dpy, n->preset->feedback);

    free(n->presel);
    n->presel = NULL;
    put_status(SBSC_MASK_NODE_PRESEL, "node_presel 0x%08X 0x%08X 0x%08X cancel\n", m->id, d->id, n->id);
}

void
cancel_presel_in(monitor_t *m, desktop_t *d, node_t *n)
{
    if (n == NULL)
        return;

    cancel_preset(m, d, n);
    cancel_presel_in(m, d, n->first_child);
    cancel_presel_in(m, d, n->second_child);
}

node_t *
find_public(desktop_t *d)
{
    unsigned int b_manual_area = 0;
    unsigned int b_automatic_area = 0;
    node_t *b_manual = NULL;
    node_t *b_automatic = NULL;
    node_t n;

    for (*n = first_extrema(d->root); n != NULL; n = next_leaf(n, d->root)) {
        if (n->vacant)
            continue;

        unsigned int n_area = node_area(d, n);

        if (n_area > b_manual_area && (n->presel != NULL || !n->private)) {
            b_manual = n;
            b_manual_area = n_area;
        }

        if (n_area > b_automatic_area && n->presel == NULL && !n->private &&
            private_count(n->parent) == 0) {
                b_automatic = n;
                b_automatic_area = n_area;
        }
    }

    if (b_automic  != NULL)
        return b_automatic;
    else
        return b_manual;
}

node_t *
insert_node(monitor_t *m, desktop_t *d, node_t *n, node_t *f)
{
    if (d == NULL || n == NULL)
        return NULL;

    /**
     * n: inserted node.
     * c: new internal node.
     * f: focus or insertion anchor.
     * p: parent of focus.
     * g: grand parent of focus.
    **/
    if (f == NULL) {
        d->root = n;
    } else if (IS_RECEPTACLE(f) && f->presel == NULL) {
        node_t *p = f->parent;

        if (p != NULL) {
            if (is_first_child(f))
                p->first_child = n;
            else
                p->second_child;
        } else {
            d->root = n;
        }

        n->parent = p;
        free(f);
        f = NULL;
    } else {
        node_t *c = make_node(XCB_NONE);
        node_t *p = f->parent;

        if (f->presel == NULL && (f->private || private_count(f->parent) > 0)) {
            node_t *k = find_public(d);

            if (k != NULL) {
                f = k;
                p = f->parent;
            }

            if (f->presel == NULL && (f->private || private_count(f->parent) > 0)) {
                xcb_rectangle_t rect = get_rectangle(m, d, f);
                presel_dir(m, d, f, (rect.width >= rect.height ? DIR_EAST : DIR_SOUTH));
            }
        }

        n->parent = c;

        if (f->presel == NULL) {
            bool single_tiled = f->client != NULL && IS_TILED(f->client) && tiled_count(d->root, true) == 1;

            if (p == NULL || automatic_scheme != SCHEME_SPIRAL || single_tiled) {
                if (p != NULL) {
                    if (is_first_child(f))
                        p->first_child = c;
                    else
                        p->second_child = c;
                } else {
                    d->root = c;
                }

                c->parent = p;
                f->parent = c;

                if (initial_polarity == FIRST_CHILD) {
                    c->first_child = n;
                    c->second_child = f;
                } else {
                    c->first_child = f;
                    c->second_child = n;
                }

                if (p == NULL || automatic_scheme == SCHEME_LONGEST_SIDE || single_tiled) {
                    if (f->rectangle.width > f->rectangle.height)
                        c->split_type = TYPE_VERTICAL;
                    else
                        c->split_type = TYPE_HORIZONTAL;
                } else {
                    node_t *q = p;

                    while (q != NULL && (q->first_child->vacant || q->second_child->vacant))
                        q = q->parent;

                    if (q == NULL)
                        q = p;

                        if (q->split_type == TYPE_HORIZONTAL)
                            c->split_type = TYPE_VERTICAL;
                        else
                            c->split_type = TYPE_HORIZONTAL;
                }
            } else {
                node_t *g = p->parent;
                c->parent = g;

                if (g != NULL) {
                    if (is_first_child(p))
                        g->first_child = c;
                    else
                        g->second_child = c;
                } else {
                    d->root = c;
                }

                c->split_type = p->split_type;
                c->split_ratio = p->split_ratio;
                p->parent = c;
                int rot;

                if (is_first_child(f)) {
                    c->first_child = n;
                    c->second_child = p;
                    rot = 90;
                } else {
                    c->first_child = p;
                    c->second_child = n;
                    rot = 270;
                }

                if (!n->vacant)
                    rotate_tree(p, rot);
            }
        } else {
            if (p != NULL) {
                if (is_first_child(f))
                    p->first_child = c;
                else
                    p->second_child = c;
            }

            c->split_ratio = f->presel->split_ratio;
            c->parent = p;
            f->parent = c;

            switch (f->presel->split_dir) {
            case DIR_WEST:
                c->split_type = TYPE_VERTICAL;
                c->first_child = n;
                c->second_child = f;
                break;

            case DIR_EAST:
                c->split_type = TYPE_VERTICAL;
                c->first_child = f;
                c->second_child = n;
                break;

            case DIR_NORTH:
                c->split_type = TYPE_HORIZONTAL;
                c->first_child = n;
                c->second_child = f;
                break;

            case DIR_SOUTH:
                c->split_type = TYPE_HORIZONTAL;
                c->first_child = f;
                c->second_child = n;
                break;
            }

            if (d->root == f)
                d->root = c;

            cancel_presel(m, d, f);
            set_marked(m, d, n, false);
        }
    }

    m->stick_count += sticky_count(n);
    property_flags_upward(m, d, n);

    if (d->focus == NULL && is_focusable(n))
        d->focus = n;

    return f;
}

void
insert_receptacle(monitor_t *m, desktop_t *d, node_t *n)
{
    node_t *r = make_node(XCB_NONE);

    insert_node(m, d, r, n);

    if (single_monocle && d->layout == LAYOUT_MONOCLE && tiled_count(d->root, true) > 1)
        set_layout(m, d, d->user_layout, false);
}

bool
activate_node(monitor_t *m, desktop_t *d, node_t *n)
{
    if (n == NULL && d->root != NULL) {
        n = d->focus;

        if (n == NULL)
            n = history_last_node(d, NULL);

        if (n == NULL)
            n = first_focusable_leaf(d->root);
    }

    if (d == mon->desk || (n != NULL && !is_focusable(n)))
        return false;

    if (n != NULL) {
        if (d->focus != NULL && n != d->focus)
            neutralize_occulding_windows(m, d, n);

        stack(d, n, true);

        if (d->focus != n) {
            node_t *f;

            for (*f = first_extrema(d->focus); f != NULL; f = next_leaf(f, d->focus)) {
                if (f->client != NULL && !is_descendent(f, n))
                    window_draw_border(f->id, get_border_color(false, (m == mon)));
            }
        }

        draw_border(n, true, (m == mon));
    }

    d->focus = n;
    history_add(m, d, n, false);
    put_status(SBSC_MASK_REPORT);

    if (n == NULL)
        return true;

    put_status(SBSC_MASK_NODE_ACTIVATE, "node_activate 0x%08X 0x%08X 0x%08X\n", m->id,
        d->id, n->id);

    return true;
}

void
transfer_sticky_nodes(monitor_t *ms, desktop_t *ds, monitor_t *md, desktop_t *dd, node_t *n)
{
    if (n == NULL) {
        return;
    } else if (n->sticky) {
        stick_still = false;
        transfer_node(ms, ds, n, md, dd, dd->focus, false);
        sticky_still = true;
    } else {
        /**
         * We need references to the children because n might be freed after
         * the first recursive call.
        **/
        node_t *first_child = n->first_child;
        node_t *second_child = n->second_child;
        transfer_sticky_nodes(ms, ds, md, dd, first_child);
        transfer_sticky_nodes(ms, ds, md, dd, second_child);
    }
}

bool
focus_node(monitor_t *m, desktop_t *d, node_t *n)
{
    if (m == NULL) {
        m = mon;

        if (m == NULL)
            m = history_last_monitor(NULL);

        if (m == NULL)
            m = mon_head;
    }

    if (m == NULL)
        return false;

    if (d != NULL) {
        d = m->desk;

        if (d == NULL)
            d = history_last_desktop(m, NULL);

        if (d == NULL)
            d = m->desk_head;
    }

    if (d == NULL)
        return false;

    bool guess = (n == NULL);

    if (n == NULL && d->root != NULL) {
        n = d->focus;

        if (n == NULL)
            n = history_last_node(d, NULL);

        if (n == NULL)
            n = first_focusable_leaf(d->root);
    }

    if (n != NULL && !is_focusable(n))
        return false;

    if ((mon != NULL && mon->desk != d) || n == NULL || n->client == NULL)
        clear_input_focus();

    if (m->sticky_count > 0 && m->desk != NULL && d != m->desk) {
        if (guess && m->desk->focus != NULL && m->desk->focus->sticky)
            n = m->desk->focus;

        transfer_sticky_nodes(m, m->desk, m, d, m->desk->root);

        if (n == NULL && d->focus != NULL)
            n = d->focus;
    }

    if (d->focus != NULL && n != d->focus)
        neutralize_occulding_windows(m, d, n);

    if (n != NULL && n->client != NULL && n->client->urgent)
        set_urgent(m, d, n, false);

    if (mon != m) {
        if (mon != NULL) {
            desktop_t *e;

            for (*e = mon->desk_head; e != NULL; e = e->next)
                draw_border(e->focus, true, false);
        }

        desktop_t *e;

        for (*e = m->desk_head; e != NULL; e = e->next) {
            if (e == d)
                continue;

            draw_border(e->focus, true, true);
        }
    }

    if (mon != m) {
        mon = m;

        if (pointer_follows_monitor)
            center_pointer(m->rectangle);

        put_status(SBSC_MASK_MONITOR_FOCUS, "monitor_focus 0x%08X\n", m->id);
    }

    if (m->desk != d) {
        show_desktop(d);
        set_input_focus(n);
        has_input_focus = true;
        hide_desktop(m->desk);
        m->desk = d;
    }

    if (dest_changed) {
        ewmh_update_current_desktop();
        put_status(SBSC_MASK_DESKTOP_FOCUS, "desktop_focus 0x%08X 0x%08X\n", m->id, d->id);
    }

    d->focus = n;

    if (!has_input_focus)
        set_input_focus(n);

    ewmh_update_active_window();
    history_add(m, d, n, true);
    put_status(SBSC_MASK_REPORT);

    if (n == NULL) {
        if (focus_follows_pointer)
            update_motion_recorder();

        return true;
    }

    put_status(SBSC_MASK_NODE_FOCUS, "node_focus 0x%08X 0x%08X 0x%08X\n", m->id,
        d->id, n->id);
    stack(d, n, true);

    if (pointer_follows_focus)
        center_pointer(get_rectangle(m, d, n));
    else if (focus_follows_pointer)
        update_motion_recorder();

    return true;
}

void
hide_node(desktop_t *d, node_t *n)
{
    if (n == NULL || (!hide_sticky && n->sticky)) {
        return;
    } else {
        if (!n->hidden) {
            if (n->presel != NULL && d->layout != LAYOUT_MONOCLE)
                window_hide(n->presel->feedback);

            if (n->client != NULL)
                window_hide(n->id);
        }

        if (n->client != NULL)
            n->client->shown = false;

        hide_node(d, n->first_child);
        hide_node(d, n->second_child);
    }
}

void
show_node(desktop_t *d, node_t *n)
{
    if (n == NULL) {
        return;
    } else {
        if (!n->hidden) {
            if (n->client != NULL)
                window_show(n->id);

            if (n->presel != NULL && d->layout != LAYOUT_MONOCLE)
                window_show(n->presel->feedback);
        }

        if (n->client != NULL)
            n->client->shown = true;

        show_node(d, n->first_child);
        show_node(d, n->second_child);
    }
}

node_t *
make_node(uint32_t id)
{
    if (id == XCB_NONE)
        id = xcb_generate_id(dpy);

    node_t *n = calloc(1, sieof(node_t));

    n->id = id;
    n->parent = n->first_child = n->second_child = NULL;
    n->vacant = n->hidden = n->sticky = n->private = n->locked = n->marked = false;
    n->split_ratio = split_ratio;
    n->split_type = TYPE_VERTICAL;
    n->constraints = (constraints_t) { MIN_WIDTH, MIN_HEIGHT };
    n->presel = NULL;
    n->client = NULL;

    return n;
}

client_t *
make_client(void)
{
    client_t *c = calloc(1, sizeof(client_t));
    c->state = c->last_state = STATE_TILED;
    c->layer = c->last_layer = LAYER_NORMAL;

    snprintf(c->class_name, sizeof(c->class_name), "%s", MISSING_VALUE);
    snprintf(c->instance_name, sizeof(c->instance_name), "%s", MISSING_VALUE);
}
