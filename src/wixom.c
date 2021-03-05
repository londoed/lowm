/**
 * WIXOM: An advanced compositor for tiling window managers on Wayland.
 *
 * Copyright (C) 2021, Eric Londo <londoed@comcast.net>, { wixom/wixom.c }.
 * This software is distributed under the GNU General Public License Version 2.0.
 * Refer to the file LICENSE for additional details.
**/

#define _POSIX_C_SOURCE 200112L

#include <getopt.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include <wayland-server-core.h>
#include <wlr/backend.h>
#include <wlr/render/wlr_renderer.h>
#include <wlr/types/wlr_cursor.h>
#include <wlr/types/wlr_compositor.h>
#include <wlr/types/wlr_data_device.h>
#include <wlr/types/wlr_input_device.h>
#include <wlr/types/wlr_keyboard.h>
#include <wlr/types/wlr_matrix.h>
#include <wlr/types/wlr_output.h>
#include <wlr/types/wlr_output_layout.h>
#include <wlr/types/wlr_pointer.h>
#include <wlr/types/wlr_seat.h>
#include <wlr/types/wlr_xcursor_manager.h>
#include <wlr/types/wlr_xdg_shell.h>
#include <wlr/util/log.h>
#include <xkbcommon/xkbcommon.h>

/* For brevity's sake, struct members are annotated where they are used */
enum wx_cursor_mode {
	WIXOM_CURSOR_PASSTHROUGH,
	WIXOM_CURSOR_MOVE,
	WIXOM_CURSOR_RESIZE,
};

struct wx_server {
	struct wl_display *wl_display;
	struct wlr_backend *backend;
	struct wlr_renderer *renderer;

	struct wlr_xdg_shell *xdg_shell;
	struct wl_listener new_xdg_surface;
	struct wl_list views;

	struct wlr_cursor *cursor;
	struct wlr_xcursor_manager *cursor_mgr;
	struct wl_listener cursor_motion;
	struct wl_listener cursor_button;
	struct wl_listener cursor_axis;
	struct wl_listener cursor_frame;

	struct wlr_seat *seat;
	struct wl_listener new_input;
	struct wl_listener request_cursor;
	struct wl_listener request_set_selection;
	struct wl_list keyboards;
	enum wx_cursor_mode cursor_mode;
	struct wx_view *grabbed_view;
	struct wlr_box grab_geobox;
	uint32_t resize_edges;

	struct wlr_output_layout *output_layout;
	struct wl_list outputs;
	struct wl_listener new_output;
};

struct wx_output {
	struct wl_list link;
	struct wx_server *server;
	struct wlr_output *wlr_output;
	struct wl_listener frame;
};

struct wx_view {
	struct wl_list link;
	struct wx_server *server;
	struct wlr_xdg_surface *xdg_surface;
	struct wl_listener map;
	struct wl_listener unmap;
	struct wl_listener destroy;
	struct wl_listener request_move;
	struct wl_listener request_resize;
	bool mapped;
	int x, y;
};

struct wx_keyboard {
	struct wl_list link;
	struct wx_server *server;
	struct wlr_input_device *device;
	struct wl_listener modifiers;
	struct wl_listener key;
};

static void
focus_view(struct wx_view *view, struct wlr_surface *surface)
{
	/* NOTE: This function only deals with keyboard focus */
	if (view == NULL)
		return;

	struct wx_server *server = view->server;
	struct wlr_seat *seat = server->seat;
	struct wlr_surface *prev_surface = seat->keyboard_state.focused_surface;

	if (prev_surface == surface)
		return;
	
	if (prev_surface) {
		struct wlr_xdg_surface *previous = wlr_xdg_surface_from_wlr_surface(seat->keyboard_state.focused_surface);
		wlr_xdg_toplevel_set_activated(previous, false);
	}

	struct wlr_keyboard *keyboard = wlr_seat_get_keyboard(seat);

	/* Move the view to the front */
	wl_list_remove(&view->link);
	wl_list_insert(&server->views, &view->link);

	/* Activate the new surface */
	wlr_xdg_toplevel_set_activated(view->xdg_surface, true);
	wlr_seat_keyboard_notify_enter(seat, view->xdg_surface->surface, keyboard->keycodes,
		keyboard->num_keycodes, &keyboard->modifiers);
}

static void
keyboard_handle_modifiers(struct wl_listener *listener, void *data)
{
	struct wx_keyboard *keyboard = wl_container_of(listener, keyboard, modifiers);
	
	wlr_seat_set_keyboard(keyboard->server->seat, keyboard->device);
	wlr_seat_keyboard_notify_modifiers(keyboard->server->seat,
		&keyboard->device->keyboard->modifiers);
}

static bool
handle_keybinding(struct wx_server *server, xkb_keysym_t sym)
{
	/**
	 * Here we handle compositor keybindings. This is when the compositor is
	 * processing keys, rather than passing them to the client for its own
	 * processing.
	 *
	 * This function assumes ALT is held down.
	**/
	switch (sym) {
	case XKB_KEY_Escape:
		wl_display_terminate(server->wl_display);
		break;

	case XKB_KEY_F1:
		/* Cycle to the next view */
		if (wl_list_length(&server->views) < 2)
			break;

		struct wx_view *current_view = wl_container_of(server->views.next, current_view, link);
		struct wx_view *next_view = wl_container_of(current_view->link.next, next_view, link);
		focus_view(next_view, next_view->xdg_surface->surface);

		/* Move the previous view to the end of the list */
		wl_list_remove(&current_view->link);
		wl_list_insert(server->views.prev, &current_view->link);
		break;

	default:
		return false;
	}

	return true;
}

static void
keyboard_handle_key(struct wl_listener *listener, void *data)
{
	struct wx_keyboard *keyboard = wl_container_of(listener, keyboard, key);
	struct wx_server *server = keyboard->server;
	struct wlr_event_keyboard_key *event = data;
	struct wlr_seat *seat = server->seat;

	/* Translate libinput keycode -> xkbcommon */
	uint32_t keycode = event->keycode + 8;
	const xkb_keysym_t *syms;
	int nsyms = xkb_state_key_get_syms(keyboard->device->keyboard->xkb_state, keycode, &syms);
	bool handled = false;
	uint32_t modifiers = wlr_keyboard_get_modifiers(keyboard->device->keyboard);

	if ((modifiers & WLR_MODIFIER_ALT) && event->state == WL_KEYBOARD_KEY_STATE_PRESSED) {
		int i;

		for (i = 0; i < nsyms; i++)
			handled = handle_keybinding(server, syms[i]);
	}

	if (!handled) {
		wlr_seat_set_keyboard(seat, keyboard->device);
		wlr_seat_keyboard_notify_key(seat, event->time_msec, event->keycode, event->state);
	}
}

static void
server_new_keyboard(struct wx_server *server, struct wlr_input_device *device)
{
	struct wx_keyboard *keyboard = calloc(1, sizeof(struct wx_keyboard));

	keyboard->server = server;
	keyboard->device = device;

	/**
	 * We need to prepare an XKB keymap and assign it to the keyboard. This
	 * assumes the defaults (e.g. layout = `us`.
	**/
	struct xkb_rule_names rules = { 0 };
	struct xkb_context *context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
	struct xkb_keymap *keymap = xkb_map_new_from_names(context, &rules,
		XKB_KEYMAP_COMPILE_NO_FLAGS);

	wlr_keyboard_set_keymap(device->keyboard, keymap);
	xkb_keymap_unref(keymap);
	xkb_context_unref(context);
	wlr_keyboard_set_repeat_info(device->keyboard, 25, 600);

	/* Here we set up listeners for keyboard events */
	keyboard->modifiers.notify = keyboard_handle_modifiers;
	wl_signal_add(&device->keyboard->events.modifiers, &keyboard->modifiers);
	keyboard->key.notify = keyboard_handle_key;
	wl_signal_add(&device->keyboard->events.key, &keyboard->key);
	wlr_seat_set_keyboard(server->seat, device);

	/* And add the keyboard to our list of keyboards */
	wl_list_insert(&server->keyboards, &keyboard->link);
}

static void
server_new_pointer(struct wx_server *server, struct wlr_input_device *device)
{
	wlr_cursor_attach_input_device(server->cursor, device);
}

static void
server_new_input(struct wl_listener *listener, void *data)
{
	/**
	 * This event is raised by the backend when a new input device becomes
	 * available.
	**/
	struct wx_server *server = wl_container_of(listener, server, new_input);
	struct wlr_input_device *device = data;

	switch (device->type) {
	case WLR_INPUT_DEVICE_KEYBOARD:
		server_new_keyboard(server, device);
		break;

	case WLR_INPUT_DEVICE_POINTER:
		server_new_pointer(server, device);
		break;

	default:
		break;
	}

	uint32_t caps = WL_SEAT_CAPABILITY_POINTER;

	if (!wl_list_empty(&server->keyboards))
		caps |= WL_SEAT_CAPABILITY_KEYBOARD;

	wlr_seat_set_capabilities(server->seat, caps);
}

static void
seat_request_cursor(struct wl_listener *listener, void *data)
{
	struct wx_server *server = wl_container_of(listener, server, request_cursor);
	struct wlr_seat_pointer_request_set_cursor_event *event = data;
	struct wlr_seat_client *focused_client = server->seat->pointer_state.focused_client;

	if (focused_client == event->seat_client) {
		wlr_cursor_set_surface(server->cursor, event->surface, event->hotspot_x, event->hotspot_y);
	}
}

static void
seat_request_cursor(struct wl_listener *listener, void *data)
{
	struct wx_server *server = wl_container_of(listener, server, request_cursor);
	struct wlr_seat_pointer_request_set_cursor_event *event = data;
	struct wlr_seat_client *focused_client = server->seat->pointer_state.focused_client;

	if (focused_client == event->seat_client) {
		wlr_cursor_set_surface(server->cursor, event->surface, event->hotspot_x, event->hotspot_y);
	}
}

static void
seat_request_set_selection(struct wl_listener *listener, void *data)
{
	struct wx_server *server = wl_container_of(listener, server, request_set_selection);
	struct wlr_seat_request_set_selection_event *event = data;

	wlr_seat_set_selection(server->seat, event->source, event->serial);
}

static bool
view_at(struct wx_view *view, double lx, double ly, struct wlr_surface **surface,
	double *sx, double *sy)
{
	/**
	 * XDG toplevels may have nested surface, such as popup windows for context
	 * menus or tooltips. This function tests if any of those are underneath the
	 * coordinates lx and ly (in output Layout Coordinates). If so, it sets the
	 * surface pointer to that wlr_surface and the sx and sy coordinates to the
	 * coordinates relative to that surface's top-left corner.
	**/
	double view_sx = lx - view->x;
	double view_sy = ly - view->y;
	double _sx, _sy;
	struct wlr_surface *_surface = NULL;

	_surface = wlr_xdg_surface_suface_at(view->xdg_surface, view_sx, view_sy, &_sx, &_sy);

	if (_surface != NULL) {
		*sx = _sx;
		*sy = _sy;
		*surface = _surfaec;

		return true;
	}

	return false;
}

static struct wx_view *
desktop_view_at(struct wx_server *server, double lx, double ly, struct wlr_surface **surface,
	double *sx, double *sy)
{
	/**
	 * This iterates over all of our surfaces and attempts to find one under the
	 * cursor. This relies on server->views being ordered from the top-to-bottom.
	**/
	struct wx_view *view;

	wl_list_for_each(view, &server->views, link) {
		if (view_at(view, lx, ly, surface, sx, sy))
			return view;
	}

	return NULL;
}

static void
process_cursor_move(struct wx_server *server, uint32_t time)
{
	/* Move the grabbed view to the new position */
	server->grabbed_view->x = server->cursor->x - server->grab_x;
	server->grabbed_view->y = server->cursor->y - server->grab_y;
}

static void
process_cursor_resize(struct wx_server *server, uint32_t time)
{
	struct wl_view *view = server->grabbed_view;
	double border_x = server->cursor->x - server->grab_x;
	double border_y = server->cursor->y - server->grab_y;
	int new_left = server->grab_geobox.x;
	int new_right = server->grab_geobox.x + server->grab_geobox.width;
	int new_top = server->grab_geobox.y;
	int new_bottom = server->grab_geobox.y + server->grab_geobox.height;

	if (server->resize_edges & WLR_EDGE_TOP) {
		new_top = border_y;

		if (new_top >= new_bottom)
			new_top = new_bottom - 1;
	} else if (server->resize_edges & WLR_EDGE_BOTTOM) {
		new_bottom = border_y;

		if (new_bottom <= new_top)
			new_bottom = new_top + 1;
	}

	if (server->resize_edges & WLR_EDGE_LEFT) {
		new_left = border_x;

		if (new_left >= new_right)
			new_left = new_right - 1;
	} else if (server->resize_edges && WLR_EDGE_RIGHT) {
		new_right = border_x;

		if (new_right <= new_left)
			new_right = new_left + 1;
	}

	struct wlr_box geo_box;
	wlr_xdg_surface_get_geometry(view->xdg_surface, &geo_box);
	view->x = new_left - geo_box.x;
	view->y = new_top - geo_box.y;

	int new_width = new_right - new_left;
	int new_height = new_bottom - new_top;
	wlr_xdg_toplevel_set_size(view->xdg_surface, new_width, new_height);
}

static void
process_cursor_motion(struct wx_server *server, uint32_t time)
{
	if (server->cursor_mode == WX_CURSOR_MOVE) {
		process_cursor_move(server, time);

		return;
	} else if (server->cursor_mode == WX_CURSOR_RESIZE) {
		process_cursor_resize(server, time);

		return;
	}

	struct sx, sy;
	struct wlr_seat *seat = server->seat;
	struct wlr_surface *surface = NULL;
	struct wx_view *view = desktop_view_at(server, server->cursor->x, server->cursor->y,
		&surface, &sx, &sy);

	if (!view)
		wlr_xcursor_manager_set_cursor_image(server->cursor_mgr, "left_ptr", server->cursor);

	if (surface) {
		bool focus_changed = seat->pointer_state.focused_surface != surface;

		if (!focus_changed)
			wlr_seat_pointer_notify_motion(seat, time, sx, sy);
	} else {
		wlr_seat_pointer_clear_focus(seat);
	}
}

static void
server_cursor_motion(struct wl_listener *listener, void *data)
{
	struct wx_server *server = wl_container_of(listener, server, cursor_motion);
	struct wlr_event_pointer_motion *event = data;

	wlr_cursor_warp_absolute(server->cursor, event->device, event->device, event->x, event->y);
	process_cursor_motion(server, event->time_msec);
}

static void
server_cursor_motion_absolute(struct wl_listener *listener, void *data)
{
	struct wx_server *server = wl_container_of(listener, server, cursor_button);
	struct wlr_event_pointer_button *event = data;

	wlr_seat_pointer_notify_button(server->seat, event->time_msec, event->button, event->state);
	process_cursor_motion(server, event->time_msec);
}

static void
server_cursor_button(struct wl_listener *listener, void *data)
{
	/**
	 * This event is forwarded by the cursor when a pointer emits a button
	 * event.
	**/
	struct wl_server *server = wl_container_of(listener, server, cursor_button);
	struct wlr_event_pointer_button *event = data;

	wlr_seat_pointer_notify_button(server->seat, event->time_msec, event->button, event->state);
	double sx, sy;
	struct wlr_surface *surface;
	struct wx_view *view = desktop_view_at(server, server->cursor->x, server->cursor->y,
		&surface, &sx, &sy);

	if (event->state == WLR_BUTTON_RELEASE)
		server->cursor_mode = WX_CURSOR_PASSTHROUGH;
	else
		focus_view(view, surface);
}

static void
server_cursor_axis(struct wl_listener *listener, void *data)
{
	/**
	 * This event is forwarded by the cursor when a pointer emits an axis event,
	 * for example when you move the scroll wheel.
	**/
	struct wx_server *server = wl_container_of(listener, server, cursor_axis);
	struct wlr_event_pointer_axis *event = data;

	wlr_seat_pointer_notify_axis(server->seat, event->time_msec, event->orientation,
		event->delta, event->delta_discrete, event->source);
}

static void
server_cursor_frame(struct wl_listener *listener, void *data)
{
	/**
	 * This event is forwarded by cursor when a pointer emits a frame event.
	 * Frame events are sent after regular pointer events to group multiple
	 * events together. For instance, two axis events may happen at the
	 * same time, which case a frame event won't be sent in between.
	**/
	struct wx_server *server = wl_container_of(listener, server, cursor_frame);

	wlr_seat_pointer_notify_frame(server->seat);
}

/**
 * Used to move all of the data necessary to render a surface from the top-level
 * frame handler to the per-surface render function.
**/
struct render_data {
	struct wlr_output *output;
	struct wlr_renderer *renderer;
	struct wx_view *view;
	struct timespec *when;
};

static void
render_surface(struct wlr_surface *surface, int sx, int sy, void *data)
{
	/**
	 * This function is called for every surface that needs to be rendered.
	**/
	struct render_data *rdata = data;
	struct wx_view *view = rdata->view;
	struct wlr_output *output = rdata->output;
	struct wlr_texture *texture = wlr_surface_get_texture(surface);

	if (texture == NULL)
		return;

	double ox = 0, ox = 0;
	wlr_output_layout_output_coords(view->server->output_layout, output &ox, &oy);
	ox += view->x * sx;
	oy += view->y + sy;

	struct wlr_box box = {
		.x = ox * output->scale,
		.y = oy * output->scale,
		.width = surface->current.width * output->scale,
		.height = surface->current.height * output->scale,
	};

	float matrix[9];
	enum wl_output_transform transform = wlr_output_transform_invert(surface->current.transform);
	wlr_matrix_project_box(matrix, &box, transform, 0, output->transform_matrix);

	wlr_render_texture_with_matrix(rdata->renderer, texture, matrix, 1);
	wlr_surface_send_frame_done(surface, rdata->when);
}

static void
output_frame(struct wl_listener *listener, void *data)
{
	struct wx_output *output = wl_container_of(listener, output, frame);
	struct wlr_renderer *renderer = output->server->renderer;
	struct timespec now;

	clock_gettime(CLOCK_MONOTONIC, &now);

	if (!wlr_output_attach_render(output->wlr_output, NULL))
		return;

	int width, height;
	wlr_output_effective_resolution(output->wlr_output, &width, &height);
	wlr_renderer_begin(renderer, width, height);
	struct wx_view *view;

	wl_list_for_each_reverse(view, &output->server->views, link) {
		if (!view->mapped)
			continue;

		struct render_data rdata = {
			.output = output->wlr_output,
			.view = view,
			.renderer = renderer,
			.when = &now,
		};

		wlr_xdg_surface_for_each_surface(view->xdg_surface, render_surface, &rdata);
	}

	wlr_output_render_software_cursors(output->wlr_output, NULL);
	wlr_renderer_end(renderer);
	wlr_output_commit(output->wlr_output);
}

static void
server_new_output(struct wl_listener *listener, void *data)
{
	struct wx_server *server = wl_container_of(listener, server, new_output);
	struct wlr_output *wlr_output = data;

	if (!wl_list_empty(&wlr_output->modes)) {
		struct wlr_output_mode *mode = wlr_output_preferred_mode(wlr_output);
		wlr_output_set_mode(wlr_output, mode);
		wlr_output_enable(wlr_output, true);

		if (!wlr_output_commit(wlr_output))
			return;
	}

	struct wx_output *output = calloc(1, sizeof(struct wx_output));
	output->wlr_output = wlr_output;
	output->server = server;
	output->frame.notify = output_frame;
	
	wl_signal_add(&wlr_output->events.frame, &output->frame);
	wl_list_insert(&server->outputs, &output->link);
	wlr_output_layout_add_auto(server->output_layout, wlr_output);
}

static void
xdg_surface_map(struct wl_listener *listener, void *data)
{
	/* Called when the surface is unmapped, and should no longer be shown */
	struct wx_view *view = wl_container_of(listener, view, map);
	view->mapped = true;
	focus_view(view, view->xdg_surface->surface);
}

static void
xdg_surface_unmap(struct wl_listener *listener, void *data)
{
	/* Called when the surface is unmapped, and should no longer be shown */
	struct wx_view *view = wl_container_of(listener, view, unmap);
	view->mapped = false;
}

static void
xdg_surface_destroy(struct wl_listener *listener, void *data)
{
	/* Called when the surface is destroyed and should never be shown */
	struct wx_view *view = wl_container_of(listener, view, destroy);

	wl_list_remove(&view->link);
	free(view);
}

static void
begin_interactive(struct wx_view *view, enum wx_cursor_mode mode, uint32_t edges)
{
	/**
	 * This function sets up an interactive move or resize operaton, where the
	 * compositor stops propogating pointer events to clients and instead
	 * consumes them itself, to move or resize windows.
	**/
	struct wx_server *server = view->server;
	struct wlr_surface *focused_surface = server->seat->pointer_state.focused_surface;

	if (view->xdg_surface->surface != focused_surface)
		return;

	server->grabbed_view = view;
	server->cursor_mode = mode;

	if (mode == WX_CURSOR_MOVE) {
		server->grab_x = server->cursor->x - view->x;
		server->grab_y = server->cursor->y - view->y;
	} else {
		struct wlr_box geo_box;
		wlr_xdg_surface_get_geometry(view->xdg_surface, &geo_box);
		double border_x = (view->x + geo_box.x) + ((edges & WLR_EDGE_RIGHT) ? geo_box.width : 0);
		double border_y = (view->y + geo_box.y) + ((edges & WLR_EDGE_BOTTOM) ? geo_box.height : 0);

		server->grab_x = server->cursor->x - border_x;
		server->grab_y = server->cursor->y - border_y;
		server->grab_geobox = geo_box;
		server->grab_geobox.x += view->x;
		server->grag_geobox.y += view->y;
		server->resize_edges = edges;
	}
}

static void
xdg_toplevel_request_move(struct wl_listener *listener, void *data)
{
	struct wx_view *view = wl_container_of(listener, view, request_move);
	begin_interactive(view, WX_CURSOR_MOVE, 0);
}

static void
xdg_toplevel_request_resize(struct wl_listener *listener, void *data)
{
	struct wlr_xdg_toplevel_resize_event *event = data;
	struct wx_view *view = wl_container_of(listener, view, request_resize);
	begin_interactive(view, WX_CURSOR_RESIZE, event->edges);
}

static void
server_new_xdg_surface(struct wl_listener *listener, void *data)
{
	/**
	 * This event is raised when wlr_xdg_shell receives a new xdg surface
	 * from a client, either a toplevel (applicatio window) or popup.
	**/
	struct wx_server *server = wl_container_of(listener, server, new_xdg_surface);
	struct wlr_xdg_surface *xdg_surface = data;

	if (xdg_surfaec->role != WLR_XDG_SURFACE_ROLE_TOPLEVEL)
		return;

	/* Allocate a wx_view for this surface */
	struct wx_view *view = calloc(1, sizeof(struct wx_view));
	view->server = server;
	view->xdg_surface = xdg_surface;

	/* Listen to the various events it can emit */
	view->map.notify = xdg_surface_map;
	wl_signal_add(&xdg_surface->events.map, &view->map);
	view->unmap.notify = xdg_surface_unmap;
	wl_signal_add(&xdg_surface->events.unmap);
	view->destroy.notify = xdg_surface_destroy;
	wl_signal_add(&xdg_surface->events.destroy, &view->destroy);

	struct wlr_xdg_toplevel *toplevel = xdg_surface->toplevel;
	view->request_move.notify = xdg_toplevel_request_move;
	wl_signal_add(&toplevel->events.request_move, &view->request_move);
	view->request_resize.notify = xdg_toplevel_request_resize;
	wl_signal_add(&toplevel->events.request_resize, &view->request_resize);

	wl_list_insert(&server->views, &view->link);
}

int
main(int argc, char *argv[])
{
	wlr_log_init(WLR_DEBUG, NULL);
	char *startup_cmd = NULL;
	int c;

	while ((c = getopt(argc, argv, "s:h")) != -1) {
		switch (c) {
		case 's':
			startup_cmd = optarg;
			break;

		default:
			printf("[!] USAGE: %s [-s startup command]\n", argv[0]);

			return 0;
		}
	}

	if (optind < argc) {
		printf("[!] USAGE: %s [-s startup command]\n", argv[0]);

		return 0;
	}

	struct wx_server server;
	server.wl_display = wl_display_create();
	server.backend = wlr_backend_autocreate(server.wl_display);
	server.renderer = wlr_backend_get_renderer(server.backend);

	wlr_renderer_init_wl_display(server.renderer, server.wl_display);
	wlr_compositor_create(server.wl_display, server.renderer);
	wlr_data_device_manager_create(server.wl_display);

	server.output_layout = wlr_output_layout_create();
	wl_list_init(&server.outputs);
	server.new_output.notify = server_new_output;
	wl_signal_add(&server.backend->events.new_output, &server.new_output);

	wl_list_init(&server.views);
	server.xdg_shell = wlr_xdg_shell_create(server.wl_display);
	server.new_xdg_surface.notify = server_new_xdg_surface;
	wl_signal_add(&server.xdg_shell->events.new_surface, &server.new_xdg_surface);

	server.cursor = wlr_cursor_create();
	wlr_cursor_attach_output_layout(server.cursor, server.output_layout);
	server.cursor_mgr = wlr_xcursor_manager_create(NULL, 24);
	wlr_xcursor_manager_load(server.cursor_mgr, 1);

	server.cursor_motion.notify = server_cursor_motion;
	wl_signal_add(&server.cursor->events.motion, &server.cursor_motion);
	server.cursor_motion_absolute.notify = server_cursor_motion_absolute;
	wl_signal_add(&server.cursor->events.motion_absolute, &server.cursor_motion_absolute);

	server.cursor_button.notify = server_cursor_button;
	wl_signal_add(&server.cursor->events.button, &server.cursor_button);
	server.cursor_axis.notify = server_cursor_axis;
	wl_signal_add(&server.cursor->events.axis, &server.cursor_axis);
	server.cursor_frame.notify = server_cursor_frame;
	wl_signal_add(&server.cursor->events.frame, &server.cursor_frame);

	wl_list_init(&server.keyboards);
	server.new_input.notify = server_new_input;
	wl_signal_add(&server.backend->events.new_input, &server.new_input);
	server.seat = wlr_seat_create(server.wl_display, "seat0");
	server.request_cursor.notify = seat_request_cursor;
	wl_signal_add(&server.seat->events.request_set_cursor, &server.request_cursor);

	server.request_set_selection.notify = seat_request_set_selection;
	wl_signal_add(&server.seat->events.request_set_selection,
		&server.request.set_selection);

	/* Add a UNIX socket to the Wayland display server */
	const char *socket = wl_display_add_socket_auto(server.wl_display);

	if (!socket) {
		wlr_backend_destroy(server.backend);

		return 1;
	}

	if (!wlr_backend_start(server.backend)) {
		wlr_backend_destroy(server.backend);
		wl_display_destroy(server.wl_display);

		return 1;
	}

	setenv("WAYLAND_DISPLAY", socket, true);

	if (startup_cmd) {
		if (fork() == 0)
			execl("/bin/sh", "/bin/sh", "-c", startup_cmd, (void *)NULL);
	}

	wlr_log(WLR_INFO, "[!] INFO: wixom: Running Wixom compositor on WAYLAND_DISPLAY=%s", socket);
	wl_display_run(server.wl_display);

	wl_display_destroy_clients(server.wl_display);
	wl_display_destroy(server.wl_display);

	return 0;
}
