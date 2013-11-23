#include <pango/pango.h>
#include <pango/pangocairo.h>
#include <cairo.h>
#include <math.h>
#include <glib.h>
#include <glib/gstdio.h>
#include "render.h"

struct GC {
	cairo_t *cairo;
	cairo_surface_t *surface;
	int width, height;
	PangoFontDescription *fontdesc;
	PangoContext *context;
	double x1, x2, y1, y2;
};

#define e 0.1

cairo_path_data_t nice_curve[] = {
	{.header = {.type=CAIRO_PATH_CURVE_TO, .length=4}},
	{.point = {.x=0.5, .y=0.25}},
	{.point = {.x=0.5, .y=0.75}},
	{.point = {.x=1.0, .y=1.0-e}}
};

cairo_path_data_t opening_curve[] = {
	{.header = {.type=CAIRO_PATH_MOVE_TO, .length=2}},
	{.point = {.x=-1.0, .y=1.0}},
	{.header = {.type=CAIRO_PATH_LINE_TO, .length=2}},
	{.point = {.x=-1.0, .y=0.0+e}},
	{.header = {.type=CAIRO_PATH_LINE_TO, .length=2}},
	{.point = {.x=0.0, .y=0.0+e}},
};

cairo_path_data_t closing_curve[] = {
	{.header = {.type=CAIRO_PATH_LINE_TO, .length=2}},
	{.point = {.x=2.0, .y=1.0-e}},
	{.header = {.type=CAIRO_PATH_LINE_TO, .length=2}},
	{.point = {.x=2.0, .y=0.0}},
};

cairo_path_t nice_path = {
	.status = CAIRO_STATUS_SUCCESS,
	.data = nice_curve,
	.num_data = sizeof(nice_curve)/sizeof(nice_curve[0])
};

cairo_path_t opening_path = {
	.status = CAIRO_STATUS_SUCCESS,
	.data = opening_curve,
	.num_data = sizeof(opening_curve)/sizeof(opening_curve[0])
};

cairo_path_t closing_path = {
	.status = CAIRO_STATUS_SUCCESS,
	.data = closing_curve,
	.num_data = sizeof(closing_curve)/sizeof(closing_curve[0])
};


/* Fixup tabs. Layout shouldn't have width defined. */
void align_tabstops_nicely(PangoLayout *layout) {
	const char *text = pango_layout_get_text(layout);
	const char *p;
	// Counting the number of tabs per line (maximal) to preallocate.
	int mtabs = 0, ntab = 0;
	gint *tabwidths, tw;
	PangoLayoutIter *iter;
	PangoTabArray *array;

	for (p = text; *p; p++) {
		if (*p == '\t') {
			ntab++;
			if (ntab > mtabs)
				mtabs++;
		}
		if (*p == '\n')
			ntab = 0;
	}
	if (mtabs == 0)
		return;
	tabwidths = g_new0(gint, mtabs);
	// index_to_pos is slow, so let's play a Pango's game.
	iter = pango_layout_get_iter(layout);
	do {
		PangoRectangle rect;
		gint lastpos;
		ntab = 0;
		pango_layout_iter_get_line_extents(iter, NULL, &rect);
		lastpos = rect.x;
		while (pango_layout_iter_get_run_readonly(iter) != NULL) {
			int idx = pango_layout_iter_get_index(iter);
			if (text[idx] == '\t') {
				gint width;
				pango_layout_iter_get_char_extents(iter, &rect);
				width = rect.x - lastpos;
				lastpos = rect.x + rect.width;
				if (width > tabwidths[ntab])
					tabwidths[ntab] = width;
				ntab++;
			}
			pango_layout_iter_next_char(iter);
		}
	} while (pango_layout_iter_next_line(iter));

	array = pango_tab_array_new(mtabs, FALSE);
	tw = 0;
	for (ntab = 0; ntab < mtabs; ntab++) {
		tw += tabwidths[ntab] + 1;
		pango_tab_array_set_tab(array, ntab, PANGO_TAB_LEFT, tw);
	}

	pango_layout_set_tabs(layout, array);
	
	pango_tab_array_free(array);
	g_free(tabwidths);
}

GC *create_context(int width, int height, int ppi) {
	GC *gc = g_new(GC, 1);
	cairo_surface_t *surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
	cairo_t *cairo = cairo_create(surface);
	PangoFontDescription *fontdesc = pango_font_description_new();
	PangoContext *context = pango_cairo_create_context(cairo);

	cairo_set_source_rgb(cairo, 1.0, 1.0, 1.0);
	cairo_rectangle(cairo, 0, 0, width, height);
	cairo_fill(cairo);

	cairo_set_source_rgb(cairo, 0.0, 0.0, 0.0);
	pango_font_description_set_family(fontdesc, "serif");
	pango_font_description_set_size(fontdesc, 11 * PANGO_SCALE);
	pango_cairo_context_set_resolution(context, (double)ppi);
	pango_cairo_update_context(cairo, context);
	

	*gc = (GC) { .surface=surface, .cairo=cairo, .width=width,
		.height=height, .fontdesc=fontdesc, .context=context,
		.x1 = 0.0, .x2 = width, .y1 = 0.0, .y2 = height
	};

	return gc;
}

void draw_nice_progression(GC *gc, double marginsize, int n) {
	const double linewidth = 2.0;
	double x, y, width, height;
	double height_of_half, width_of_one;
	int i;
	cairo_matrix_t matrix;
	PangoFontDescription *fontdesc = pango_font_description_new();
	PangoLayout *layout = pango_layout_new(gc->context);

	rmargin(gc, marginsize);
	cairo_identity_matrix(gc->cairo);
	x = gc->x2 + linewidth;
	y = gc->y1 + linewidth;
	height = gc->y2 - 2 * linewidth;
	width = marginsize - 2 * linewidth;

	height_of_half = height / (n + 1);
	width_of_one = width / 3.0;
	// Draw a zig zag
	cairo_set_line_width(gc->cairo, linewidth);
	cairo_set_line_cap(gc->cairo, CAIRO_LINE_CAP_ROUND);
	cairo_set_line_join(gc->cairo, CAIRO_LINE_JOIN_ROUND);
	for (i = 0; i < n + 1; i ++) {
		double xorig = x + width_of_one, xx = width_of_one;
		if (i % 2) {
			xorig += width_of_one;
			xx = -xx;
		}
		cairo_matrix_init(&matrix,
			xx, 0.0, 0.0, height_of_half, xorig, y + i * height_of_half);
		cairo_set_matrix(gc->cairo, &matrix);
		if (i != n) {
			cairo_append_path(gc->cairo, &opening_path);
		} else {
			cairo_move_to(gc->cairo, -1.0, 0.0 + e);
			cairo_line_to(gc->cairo, 0.0, 0.0 + e);
		}
		cairo_append_path(gc->cairo, &nice_path);
		if (i) {
			cairo_append_path(gc->cairo, &closing_path);
		} else {
			cairo_line_to(gc->cairo, 2.0, 1.0 - e);
		}
		cairo_identity_matrix(gc->cairo);
		cairo_stroke(gc->cairo);
	}

	pango_font_description_set_family(fontdesc, "sans");
	pango_font_description_set_absolute_size(fontdesc, pango_units_from_double(height_of_half));
	pango_layout_set_font_description(layout, fontdesc);
	for (i = 1; i <= n; i++) {
		PangoRectangle extents;
		double xorig = x + width_of_one / 2;
		double yorig = y + height_of_half * i;
		double w, h;
		if (i % 2 == 0)
			xorig = x + marginsize - width_of_one / 2;
		char *si = g_strdup_printf("%d", i);
		pango_layout_set_text(layout, si, -1);
		g_free(si);
		pango_layout_get_extents(layout, NULL, &extents);
		w = pango_units_to_double(extents.width);
		h = pango_units_to_double(extents.height);
		if (i % 2 == 0)
			xorig -= w;
		cairo_move_to(gc->cairo, xorig, yorig - h/2);
		pango_cairo_show_layout(gc->cairo, layout);
	}
	g_object_unref(layout);
	pango_font_description_free(fontdesc);
}

void draw_bubble_progression(GC *gc, int n, int multiply) {
	const double step_scale = 2.2, space_scale = 0.2;
	double x, y, width, height, radius, step, space;
	int i;
	PangoFontDescription *fontdesc = pango_font_description_new();
	PangoLayout *layout = pango_layout_new(gc->context);
	height = gc->y2 - gc->y1;
	y = gc->y1;
	// Height = n * step + n-1 * space;
	radius = height / (step_scale * n + space_scale * (n - 1));
	step = radius * step_scale;
	space = radius * space_scale;
	width = 2.4 * radius;
	rmargin(gc, width);
	x = gc->x2 + width/2;
	pango_font_description_set_family(fontdesc, "sans");
	pango_font_description_set_absolute_size(fontdesc, pango_units_from_double(radius * 0.7));
	pango_layout_set_font_description(layout, fontdesc);
	cairo_set_line_width(gc->cairo, 2.0);
	for (i = 1; i <= n; i++) {
		PangoRectangle extents;
		double yorig = y + step * i + space * (i - 1) - step / 2;
		double w, h;
		char *si = g_strdup_printf("%d", i * multiply);
		pango_layout_set_text(layout, si, -1);
		g_free(si);

		cairo_new_sub_path(gc->cairo);
		cairo_arc(gc->cairo, x, yorig, radius, 0, 2 * M_PI);
		cairo_stroke(gc->cairo);

		pango_layout_get_extents(layout, NULL, &extents);
		w = pango_units_to_double(extents.width);
		h = pango_units_to_double(extents.height);
		cairo_move_to(gc->cairo, x - w/2, yorig - h/2);
		pango_cairo_show_layout(gc->cairo, layout);
	}
	g_object_unref(layout);
	pango_font_description_free(fontdesc);
}

void lmargin(GC *gc, double margin) {
	gc->x1 += margin;
}

void rmargin(GC *gc, double margin) {
	gc->x2 -= margin;
}

void tmargin(GC *gc, double margin) {
	gc->y1 += margin;
}

void bmargin(GC *gc, double margin) {
	gc->y2 -= margin;
}

void render_text_nicely(GC *gc, const char *text) {
	PangoLayout *layout;
	PangoRectangle rect;
	layout = pango_layout_new(gc->context);
	pango_layout_set_font_description(layout, gc->fontdesc);
	pango_layout_set_markup(layout, text, -1);
	align_tabstops_nicely(layout);
	pango_layout_set_ellipsize(layout, PANGO_ELLIPSIZE_END);
	pango_layout_set_wrap(layout, PANGO_WRAP_WORD_CHAR);
	pango_layout_set_height(layout, pango_units_from_double(gc->y2 - gc->y1));
	pango_layout_set_width(layout, pango_units_from_double(gc->x2 - gc->x1));
	cairo_move_to(gc->cairo, gc->x1, gc->y1);
	pango_cairo_show_layout(gc->cairo, layout);
	pango_layout_get_extents(layout, NULL, &rect);
	tmargin(gc, pango_units_to_double(rect.height));
	g_object_unref(layout);
	pango_cairo_update_context(gc->cairo, gc->context);
}

static
cairo_status_t simple_png_writer(void *cl, const unsigned char *data, unsigned int length) {
	FILE *f = (FILE *)cl;
	fwrite(data, 1, length, f);
	if (ferror(f)) {
		return CAIRO_STATUS_WRITE_ERROR;
	}
	return CAIRO_STATUS_SUCCESS;
}

void save_to_png(GC *gc, const char *filename) {
	// Sadly, on Windows, things are broken (probably by GLib?), but they are fixed (probably by GLib, too).
	FILE *f = g_fopen(filename, "wb");
	cairo_surface_write_to_png_stream(gc->surface, simple_png_writer, f);
	fclose(f);
}

void destroy_context(GC *gc) {
	cairo_surface_destroy(gc->surface);
	cairo_destroy(gc->cairo);
	pango_font_description_free(gc->fontdesc);
	g_object_unref(gc->context);
}
