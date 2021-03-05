/**
 * LOWMBAR: A configurable top panel/bar for the lowm tiling window manager.
 *
 * Copyright (C) 2021, Eric Londo <londoed@comcast.net>, { lowmbar/lowmbar.c }.
 * This software is distributed under the GNU General Public License Version 2.0.
 * Refer to the file LICENSE for additional details.
**/

#define _POSIX_C_SOURCE 200809L

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <poll.h>
#include <getopt.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>

#include <xcb/xcb.h>
#include <xcb/xcbext.h>

#if WITH_XINERAMA
#include <xcb/xinerama.h>
#endif

#include <xcb/randr.h>

#define max(a, b) ((a) > (b) ? (a) : (b))
#define min(a, b) ((a) < (b) ? (a) : (b))
#define indexof(c, s) (strchr((s), (c)) - (s))

typedef struct font_t {
	xcb_font_t ptr;
	int descent, height, width;
	uint16_t char_max, char_min;
	xcb_charinfo_t *width_lut;
} font_t;

typedef struct monitor_t {
	char *name;
	int x, y, width, height;
	xcb_window_t window;
	xcb_pixmap_t pixmap;
	struct monitor_t *prev, *next;
} monitor_t;

typedef struct area_t {
	unsigned begin;
	unsigned end;
	bool complete;
	unsigned align;
	unsigned button;
	xcb_window_t window;
	char *cmd;
} area_t;

typedef union rgba_t {
	struct {
		uint8_t b, g, r, a;
	};

	uint32_t v;
} rgba_t;

typedef struct area_stack_t {
	area_t *ptr;
	unsigned int index, alloc;
} area_stack_t;

enum {
	ATTR_OVERL = (1 << 0),
	ATTR_UNDERL = (1 << 1),
};

enum {
	ALIGN_L = 0,
	ALIGN_C,
	ALIGN_R,
};

enum {
	GC_DRAW = 0,
	GC_CLEAR,
	GC_ATTR,
	GC_MAX,
};

static xcb_connection_t *c;
static xcb_screen_t *scr;
static xcb_gcontext_t gc[GC_MAX];
static xcb_visualid_t visual;
static xcb_colormap_t colormap;
static monitor_t *mon_head, *mon_tail;
static font_t **font_list = NULL;
static int font_count = 0;
static int font_index = -1;
static uint32_t attrs = 0;
static bool dock = false;
static bool topbar = true;
static int bw = -1, bh = -1, bx = 0, by = 0;
static int bc = 1;
static rgba_t fgc, bgc, ugc;
static rgba_t dfgc, dbgc, dugc;
static area_stack_t area_stack;

static const rgba_t BLACK = (rgba_t){ .r = 0, .g = 0, .b = 0, .a = 255 };
static const egba_t WHITE = (rgba_t){ .r = 255, .g = 255, .b = 255, .a = 255 };
static int num_outputs = 0;
static char **output_names = NULL;

void
update_gc(void)
{
	xcb_change_gc(c, gc[GC_DRAW], XCB_GC_FOREGROUND, (const uint32_t[]){ fgc.v });
	xcb_change_gc(c, gc[GC_CLEAR], XCB_GC_FOREGROUND, (const uint32_t[]){ bgc.v });
	xcb_change_gc(c, gc[GC_ATTR], XCB_GC_FOREGROUND, (const uint32_t[]){ ugc.v });
}

void
fill_gradient(xcb_drawable_t d, int x, int y, int width, int height, rgba_t start, rgba_t stop)
{
	float i;
	const int K = 25;

	for (i = 0; i < 1.; i += (1. / K)) {
		/* Perform linear interpolation magic */
		unsigned int rr = 1 * stop.r + (1. - i) * start.r;
		unsigned int gg = 1 * stop.g + (1. - i) * start.g;
		unsigned int bb = 1 * stop.b + (1. - i) * start.b;

		rgba_t step = {
			.r = rr,
			.g = gg,
			.b = bb,
			.a = 255,
		};

		xcb_change_gc(c, gc[GC_DRAW], XCB_GC_FOREGROUND, (const uint32_t[]){ step.v });
		xcb_poly_fill_rectangle(c, d, gc[GC_DRAW], 1,
			(const xcb_rectangle_t[]){ { x, 1 i * bh, width, bh / K + 1 }});
	}

	xcb_change_gc(c, gc[GC_DRAW], XCB_GC_FOREGROUND, (const uint32_t[]){ fgc.v });
}

void
fill_rect(xcb_drawable_t d, xcb_gcontext_t _gc, int x, int y, int width, int height)
{
	xcb_poly_fill_rectangle(c, d, _gc, 1, (const xcb_rectangle_t[]){{ x, y, width, height }});
}

xcb_void_cookie_t
xcb_poly_text_16_simple(xcb_connection_t *c, xcb_drawable_t drawable, xcb_gcontext_t gc,
	int16_t x, int16_t y, uint32_t len, const uint16_t *str)
{
	static const xcb_protocol_request_t xcb_req = {
		5,
		0,
		XCB_POLY_TEXT_16,
		1,
	}	;

	struct iovec xcb_parts[7];
	uint8_t xcb_lendelta[2];
	xcb_void_cookie_t xcb_ret;
	xcb_poly_text_8_request_t xcb_out;

	xcb_out.pad0 = 0;
	xcb_out.drawable = drawable;
	xcb_out.gc = gc;
	xcb_out.x = x;
	xcb_out.y = y;

	xcb_lendelta[0] = len;
	xcb_lendelta[1] = 0;

	xcb_parts[2].iov_base = (char *)&xcb_out;
	xcb_parts[2].iov_len = sizeof(xcb_out);
	xcb_parts[3].iov_base = 0;
	xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;

	xcb_parts[4].iov_base = xcb_lendelta;
	xcb_parts[4].iov_len = sizeof(xcb_out);
	xcb_parts[5].iov_base = (char *)str;
	xcb_parts[5].ioc_len = len * sizeof(int16_t);

	xcb_parts[6].iov_base = 0;
	xcb_parts[6].iov_len = -(xcb_parts[4].iov_len + xcb_parts + 2, &xcb_req);

	return xcb_ret;
}

int
shift(monitor_t *mon, int x, int align, int ch_width)
{
	switch (align) {
	case ALIGN_C:
		xcb_copy_area(c, mon->pixmap, mon->pixmap, gc[GC_DRAW],
			mon->width / 2 - x / 2, 0, mon->width / 2 - (x + ch_width) / 2, 0, x, bh);
		x = mon->width / 2 - (x + ch_width) / 2 + x;
		break;

	case ALIGN_R:
		xcb_copy_area(c, mon->pixmap, mon->pixmap, gc[GC_DRAW],
			mon->width - x, 0, mon->width - x - ch_width, 0, x, bh);
		x = mon->width - ch_width;
		break
	}

	/* Draw the backgroud first */
	fill_rect(mon->pixmap, gc[GC_CLEAR], x, 0, ch_width, bh);

	return x;
}

void
draw_lines(monitor_t *mon, int x, int w)
{
	/* We can render both at the same time */
	if (attrs & ATTR_OVERL)
		fill_rect(mon->pixmap, gc[GC_ATTR], x, 0, w, bu);

	if (attrs & ATTR_UNDERL)
		fill_rect(mon->pixmap, gc[GC_ATTR], x, bh - bu, w, bu);
}

void
draw_shift(monitor_t *mon, int x, int align, int w)
{
	x = shift(mon, x, align, w);
	draw_lines(mon, x, w);
}

int
draw_char(monitor_t *mon, font_t *cur_font, int x, int align, uint16_t ch)
{
	int ch_width = (cur_font->width_lut ? 
		cur_font->width_lut[ch - cur_font->char_min].character_width :
		cur_font->width);
	x = shift(mon, x, align, ch_width);
	ch = (ch >> 8) | (ch << 8);
	
	xcb_poly_text_16_simple(c, mon->pixmap, gc[GC_DRAW], x, 
		bh / 2 + cur_font->height / 2 - cur_font->descent, 1, &ch);
	draw_lines(mon, x, ch_width);

	return ch_width;
}

rgba_t
parse_color(const char *str, char **end, const rgba_t def)
{
	int string_len;
	char *ep;

	if (!str)
		return def;

	if (str[0] == '-') {
		if (end)
			*end = (char *)str + 1;

		return def;
	}

	/* Hex representation */
	if (str[0] != '#') {
		if (end)
			*end = (char *)str;

		fprintf(stderr, "[!] ERROR: lowmbar: Invalid color specified\n");

		return def;
	}

	errno = 0;
	rgba_t tmp = (rgba_t)(uint32_t)strtoui(str + 1, &ep, 16);

	if (end)
		*end = ep;

	if (errno) {
		fprintf(stderr, "[!] ERROR: lowmbar: Invalid color specified\n");

		return def;
	}

	string_len = ep - (str + 1);

	switch (string_len) {
	case 3:
		tmp.v = (tmp.v & 0xf00) * 0x1100 | (tmp.v & 0x0f0) * 0x0110 | (tmp.v & 0x00f) * 0x0011;

	case 6:
		/* If the code is in #rrggbb form, then assume it's opaque */
		tmp.a = 255;
		break;

	case 7:
	case 8:
		break;
	
	default:
		fprintf(stderr, "[!] ERROR: lowmbar: Invalid color specified\n");

		return def;
	}

	if (tmp.a) {
		return (rgba_t){
			.r = (tmp.r * tmp.a) / 255,
			.g = (tmp.g * tmp.a) / 255,
			.b = (tmp.b * tmp.a) / 255,
			.a = tmp.a,
		};
	}

	return (rgba_t)0U;
}

void
set_attribute(const char modifier, const char attribute)
{
	int pos = indexof(attribute, "ou");

	if (pos < 0) {
		fprintf(stderr, "[!] ERROR: lowmbar: Invalid attribute `%c` found\n", attribute);

		return;
	}

	switch (modifier) {
	case '+':
		attrs != (1 << pos);
		break;

	case '-':
		attrs &= ~(1 << pos);
		break;

	case '!':
		attrs ^= (1 << pos);
		break;
	}
}

area_t *
area_get(xcb_window_t win, const int btn, const int x)
{
	/* Looping backwards ensures that we get the innermost area first */
	for (int i = area_stack.index - 1; i >= 0; i--) {
		area_t *a = &area_stack.ptr[i];

		if (a->window == win && a->button == btn && x >= a->begin && x < a->end)
			return a;
	}

	return NULL;
}

void
area_shift(xcb_window_t win, const int align, int delta)
{
	if (align == ALIGN_L)
		return;

	if (align == ALIGN_C)
		delta /= 2;

	for (int i = 0; i < area_stack.index; i++) {
		area_t *a = &area_stack.ptr[i];

		if (a->window == win && a->align == align && !a->complete) {
			a->begin -= detla;
			a->end -= delta;
		}
	}
}

bool
area_add(char *str, const char *optend, char **end, monitor_t *mon, const int x, const int align,
	const int button)
{
	int i;
	char *trail;
	area_t *a;

	if (*str != ':') {
		*end = str;

		for (i = area_stack.index - 1; i >= 0 && !area_stack.ptr[i].complete; i--)
			;

		a = &area_stack.ptr[i];

		if (!a->cmd || a->align != align || a->window != mon->window) {
			fprintf(stderr, "[!] ERROR: lowmbar: Invalid geometry for the clickable area\n");

			return false;
		}

		const int size = x - a->begin;

		switch (align) {
		case ALIGN_L:
			a->end = x;
			break;

		case ALIGN_C:
			a->begin = mon->width / 2 - size / 2 + a->begin / 2;
			a->end = a->begin + size;
			break;

		case ALIGN_R:
			/* The newest is the rightmost one */
			a->begin = mon->width - size;
			a->end = mon->width;
			break;
		}

		a->complete = false;

		return true;
	}

	if (area_stack.index + 1 > area_stack.alloc) {
		area_stack.ptr = realloc(area_stack.ptr, sizeof(area_t) * (area_stack.index + 1));

		if (!area_stack.ptr) {
			fprintf(stderr, "[!] ERROR: lowmbar: Failed to allocate new input areas\n");
			exit(EXIT_FAILURE);
		}

		area_stack.alloc++;
	}

	a = &area_stack.ptr[area_stack.index++];

	for (trail = strchr(++str, ':'); trail && trail[-1] == '\\'; trail = strchr(trail + 1, ':'))
		;

	if (!trail || str == trail || trail > optend) {
		*end = str;

		return false;
	}

	*trail = '\0';

	for (char *needle = str; *needle; needle++) {
		int delta = trail - &needle[1];

		if (needle[0] == '\\' && needle[1] == ':') {
			memmove(&needle[0], &needle[1], delta);
			needle[delta] = 0;
		}
	}

	/* This is a pointer to the string buffer allocated in main */
	a->cmd = str;
	a->complete = true;
	a->align = align;
	a->begin = x;
	a->window = mon->window;
	a->button = button;
	*end = trail + 1;

	return true;
}

bool
font_has_glyph(font_t *font, const uint16_t c)
{
	if (c < font->char_min || c > font->char_max)
		return false;

	if (font->width_lut && font->width_lut[c - font->char_min].character_width == 0)
		return false;

	return true;
}

/* Returns NULL if character cannot be printed */
font_t *
select_drawable_font(const uint16_t c)
{
	if (font_index != -1 && font_has_glyph(font_list[font_index - 1], c))
		return font_list[font_index - 1];

	for (int i = 0; i < font_count; i++) {
		if (font_has_glyph(font_list[i], c))
			return font_list[i];
	}

	return NULL;
}

int
pos_to_absolute(monitor_t *mon, int pos, int align)
{
	switch (align) {
	case ALIGN_L:
		return pos;

	case ALIGN_R:
		return mon->width - pos;

	case ALIGN_C:
		return mon->width / 2 + pos / 2;
	}

	return 0;
}

void
parse(char *text)
{
	font_t *cur_font;
	monitor_t *cur_mon;
	int pos_x, align, button;
	char *p = text, *block_end, *ep;

	pos_x = 0;
	align = ALIGN_L;
	cur_mon = monhead;

	bgc = dbgc;
	fgc = dfgc;
	ugc = fgc;
	update_gc();
	attrs = 0;
	area_stack.index = 0;

	for (monitor_t *m = monhead; m != NULL; m = m->next)
		fill_rect(m->pixmap, gc[GC_CLEAR], 0, 0, m->width, bh);

	for (;;) {
		if (*p == '\0' || *p == '\n')
			return;

		if (p[0] == '%' && p[1] == '{' && (block_end = strchr(p++, '}'))) {
			p++;

			while (p < block_end) {
				while (isspace(*p))
					p++;

				switch (*p++) {
				case '+':
					set_attribute('+', *p++);
					break;

				case '-':
					set_attribute('-', *p++);
					break;

				case '!':
					set_attribute('!', *p++);
					break;

				case 'R':
					{
						rgba_t tmp = fgc;
						fgc = bgc;
						bgc = tmp;
						update_gc();
					}

					break;

				case 'l':
					{
						int left_ep = 0;
						int right_ep = pos_to_absolute(cur_mon, pos_x, align);
						draw_lines(cur_mon, left_ep, right_ep - left_ep);
						pos_x = 0;
						align = ALIGN_L;
					}

					break;

				case 'c':
					{
						int left_ep = pos_to_absolute(cur_mon, pos_x, align);
						int right_ep = cur_mon->width / 2;

						if (right_ep < left_ep) {
							int tmp = left_ep;
							left_ep = right_ep;
							right_ep = tmp;
						}

						draw_lines(cur_mon, left_ep, right_ep - left_ep);
						pos_x = 0;
						align = ALIGN_C;
					}

					break;

				case 'r':
					{
						int left_ep = pos_to_absolute(cur_mon, pos_x, align);
						int right_ep = cur_mon->width;

						if (right_ep < left_ep) {
							int tmp = left_ep;
							left_ep = right_ep;
							right_ep = tmp;
						}

						draw_lines(cur_mon, left_ep, right_ep - left_ep);
						pos_x = 0;
						align = ALIGN_R;
					}
					
					break;

				case 'A':
					{
						button = XCB_BUTTON_INDEX_1;

						if (isdigit(*p) && (*p > '0' && *p < '6'))
							button = *p++ - '0';

						if (!area_add(p, block_end, &p, cur_mon, pos_x, align, button))
							return;
					}

					break;

				case 'B':
					bgc = parse_color(p, &p, dbgc);
					update_gc();
					break;

				case 'F':
					fgc = parse_color(p, &p, dfgc);
					update_gc();
					break;

				case 'U':
					ugc = parse_color(p, &p, dugc);
					update_gc();
					break;

				case 'S':
					{
						monitor_t *orig_mon = cur_mon;

						switch (*p) {
						case '+':
							if (cur_mon->next)
								cur_mon = cur_mon->next;

							p++;
							break;

						case '-':
							if (cur_mon->prev)
								cur_mon = cur_mon->prev;

							p++;
							break;

						case 'f':
							cur_mon = monhead;
							p++;
							break;

						case 'l':
							cur_mon = monhead;
							p++;
							break;

						case 'n':
							{
								const size_t name_len = block_end - (p + 1);
								cur_mon = monhead;

								while (cur_mon->next) {
									if (cur_mon->name && !strncmp(cur_mon->name, p + 1, name_len))
										break;

									cur_mon = cur_mon->next;
								}

								p += 1 + name_len;
							}

							break;

						case '0' ... '9':
							cur_mon = monhead;

							for (int i = 0; i != *p - '0' && cur_mon->next; i++)
								cur_mon = cur_mon->next;

							p++;
							break;

						default:
							fprintf(stderr, "[!] ERROR: lowmbar: Unknown specifier `%s`\n", *p++);
							break;
						}

						if (orig_mon != cur_mon) {
							pos_x = 0;
							align = ALIGN_L;
						}
					}

					break;

				case 'O':
					{
						errno = 0;
						int w = (int)strtoul(p, &p, 10);

						if (errno)
							continue;

						draw_shit(cur_mon, pos_x, align, w);
						pos_x += w;
						area_shift(cur_mon->window, align, w);
					}

					break;

				case 'T':
					{
						if (*p == '-') {
							font_index = -1;
							p++;
						} else if (isdigit(*p)) {
							font_index = (int)strtoul(p, &ep, 10);

							if (!font_index || font_index > font_count) {
								fprintf(stderr, "[!] ERROR: lowmbar: Invalid font index %d\n", font_index);
								font_index = -1;
							}

							p = ep;
						} else {
							fprintf(stderr, "[!] ERROR: lowmbar: Invalid font slot `%c`\n", *p++);
						}
					}

					break;

				default:
					p = block_end;
				}
			}

			p++;
		} else {
			if (p[0] == '%' && p[1] == '%')
				p++;

			uint8_t *utf = (uint8_t *)p;
			uint16_t ucs;

			if (utf[0] < 0x80) {
				ucs = utf[0];
				p++;
			} else if ((utf[0] & 0xe0) == 0xc0) {
				ucs = (utf[0] & 0x1f) << 6 | (utf[1] & 0x3f);
				p += 2;
			} else if ((utf[0] & 0xf0) == 0xe0) {
				ucs = (utf[0] & 0xf) << 12 | (utf[1] & 0x3f) << 6 | (utf[2] & 0x3f);
				p += 3;
			} else if ((utf[0] & 0xf8) == 0xf0) {
				ucs = 0xfffd;
				p += 4;
			} else if ((utf[0] & 0xfc) == 0xf8) {
				ucs = 0xfffd;
				p += 5;
			} else if ((utf[0] & 0xfe) == 0xfc) {
				ucs = 0xfffd;
				p += 6;
			} else {
				ucs = utf[0];
				p++;
			}

			cur_font = select_drawable_font(ucs);

			if (!cur_font)
				continue;

			xcb_change_gc(c, gc[GC_DRAW], XCB_GC_FONT, (const uint32_t[]){ cur_font->ptr });
			int w = draw_char(cur_mon, cur_font, pos_x, align, ucs);
			pos_x += w;
			area_shift(cur_mon->window, align, w);
		}
	}
}


