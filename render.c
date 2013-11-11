#include <pango/pango.h>
#include <pango/pangocairo.h>
#include <cairo.h>
#include "render.h"

struct GC {
	cairo_t *cairo;
	cairo_surface_t *surface;
	int width, height;
	PangoFontDescription *fontdesc;
	PangoContext *context;
	PangoRectangle content;
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

GC *create_context(int width, int height, int hmarg, int vmarg, int ppi) {
	GC *gc = g_new(GC, 1);
	cairo_surface_t *surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
	cairo_t *cairo = cairo_create(surface);
	PangoFontDescription *fontdesc = pango_font_description_new();
	PangoContext *context = pango_cairo_create_context(cairo);

	cairo_set_source_rgb(cairo, 1.0, 1.0, 1.0);
	cairo_rectangle(cairo, 0, 0, width, height);
	cairo_fill(cairo);

	cairo_set_source_rgb(cairo, 0.0, 0.0, 0.0);
	cairo_move_to(cairo, (double) hmarg, (double)vmarg);
	pango_font_description_set_family(fontdesc, "serif");
	pango_font_description_set_weight(fontdesc, PANGO_WEIGHT_NORMAL);
	pango_font_description_set_size(fontdesc, 11 * PANGO_SCALE);
	pango_cairo_context_set_resolution(context, (double)ppi);
	pango_cairo_update_context(cairo, context);
	

	*gc = (GC) { .surface=surface, .cairo=cairo, .width=width,
		.height=height, .fontdesc=fontdesc, .context=context,
		.content=(PangoRectangle) {
			hmarg * PANGO_SCALE, vmarg * PANGO_SCALE,
			(width - 2*hmarg) * PANGO_SCALE,
			(height - 2*vmarg) * PANGO_SCALE
		}
	};

	return gc;
}

void render_text_nicely(GC *gc, const char *text) {
	PangoLayout *layout;
	PangoRectangle rect;
	layout = pango_layout_new(gc->context);
	pango_layout_set_font_description(layout, gc->fontdesc);
	pango_layout_set_markup(layout, text, -1);
	align_tabstops_nicely(layout);
	pango_layout_set_width(layout, gc->content.width);
	pango_cairo_show_layout(gc->cairo, layout);
	pango_layout_get_extents(layout, NULL, &rect);
	cairo_rel_move_to(gc->cairo, 0.0, ((double)(rect.y + rect.height)) / PANGO_SCALE);
	g_object_unref(layout);
	pango_cairo_update_context(gc->cairo, gc->context);
}

void save_to_png(GC *gc, const char *filename) {
	cairo_surface_write_to_png(gc->surface, filename);
}

void destroy_context(GC *gc) {
	cairo_surface_destroy(gc->surface);
	cairo_destroy(gc->cairo);
	pango_font_description_free(gc->fontdesc);
	g_object_unref(gc->context);
}
