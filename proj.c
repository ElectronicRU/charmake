#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>
#include "strife.h"
#include "render.h"
#include "skills.h"

G_MODULE_EXPORT
void on_window_destroy (GObject *object, gpointer user_data) {
        gtk_main_quit();
}

const int a6_width = 827, a6_height = 1165; // 148x105mm, 200dpi
const int margin_v = 100, margin_h = 125; // 0.5", 0.625", 200dpi

const gint damage_multipliers[] = {0, 1, 2, 4, 5, 6, 7, 8};
const gint damage_denominator = 4;
void render_save(GtkBuilder *builder, const char *fname);

G_MODULE_EXPORT
void invoke_save(GObject *stupid_button, GtkBuilder *builder) {
	GtkWidget *dialog = gtk_file_chooser_dialog_new("Save as...",
			GTK_WINDOW(gtk_builder_get_object(builder, "window")),
			GTK_FILE_CHOOSER_ACTION_SAVE,
			GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
			NULL);
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog),
			GTK_FILE_FILTER(gtk_builder_get_object(builder, "filefilter")));
	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
		char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
		render_save(builder, filename);
		g_free(filename);
	}

	gtk_widget_destroy(dialog);
}

const gchar *combo_field(GtkBuilder *builder, const gchar *name, gint field,
		GStringChunk *pool) {
	GtkComboBox *box = GTK_COMBO_BOX(gtk_builder_get_object(builder, name));
	GtkTreeModel *model = gtk_combo_box_get_model(box);
	GtkTreeIter iter;
	gchar *s, *r;
	gtk_combo_box_get_active_iter(box, &iter);
	gtk_tree_model_get(model, &iter, field, &s, -1);
	r = g_string_chunk_insert(pool, s);
	g_free(s);
	return r;
}

const gchar *entry_text(GtkBuilder *builder, const gchar *name) {
	return gtk_entry_get_text(
			GTK_ENTRY(gtk_builder_get_object(builder, name))
	);
}

gint spin_value(GtkBuilder *builder, const gchar *name) {
	return gtk_spin_button_get_value_as_int(
			GTK_SPIN_BUTTON(gtk_builder_get_object(builder, name))
	);
}

void render_save(GtkBuilder *builder, const char *fname) {
	GString *result = g_string_new("");
	GStringChunk *pool = g_string_chunk_new(1024);
	GC *gc = create_context(a6_width, a6_height, 200);

	draw_nice_progression(gc, margin_h, 50);
	draw_bubble_progression(gc, 10, 50);
	tmargin(gc, margin_v); bmargin(gc, margin_v);
	lmargin(gc, margin_h);

	{
		g_string_printf(result, "%s, %s %d уровня\n",
				entry_text(builder, "ent_name"),
				entry_text(builder, "ent_class"),
				spin_value(builder, "spin_level"));
		g_string_append_printf(result, "ЗД: \t%d\nУст: \t%d\nОД: \t%d\n",
				spin_value(builder, "spin_hp"),
				spin_value(builder, "spin_fgue"),
				spin_value(builder, "spin_ap"));
		render_text_nicely(gc, result->str);
	}
	{
		gint damage = spin_value(builder, "spin_wpn_dmg");
		int i;
		const gchar *label;
		g_string_printf(result, "Атака: \t%d \t%s",
				damage,
				combo_field	(builder, "combo_wpn_dmtype", SHORT, pool));
		if ((label = entry_text(builder, "entry_weapon"))[0]) { // not empty
			g_string_append_printf(result, ", \t%s", label);
		}
		g_string_append_c(result, '\n');
		for (i = 0; i < G_N_ELEMENTS(damage_multipliers); i++) {
			gint dmg = (damage * damage_multipliers[i])/damage_denominator;
			g_string_append_printf(result, i?" | %d":"%d", dmg);
		}
		g_string_append_c(result, '\n');
		g_string_append_printf(result, "Защита: \t%d \t%s",
				spin_value	(builder, "spin_def_value"),
				combo_field	(builder, "combo_def_type", SHORT, pool));
		if ((label = entry_text(builder, "entry_armour"))[0]) { // not empty
			g_string_append_printf(result, ", \t%s", label);
		}
		g_string_append_c(result, '\n');
		render_text_nicely(gc, result->str);
	}
	{
		GtkTreeModel *model = GTK_TREE_MODEL(gtk_builder_get_object(builder, "treestore_skills"));
		GtkTreeIter iter_p, iter_c;

		g_string_assign(result, "");
		gtk_tree_model_get_iter_first(model, &iter_p);
		do {
			gchar *name, *temp;
			if (!gtk_tree_model_iter_children(model, &iter_c, &iter_p))
				continue;
			gtk_tree_model_get(model, &iter_p, NAME, &name, -1);
			// Unicode magic.
			temp = g_utf8_offset_to_pointer(name, 3);
			g_string_append_printf(result, "<b>%.*s:\t</b>", temp - name, name);
			g_free(name);
			do {
				gint level;
				gtk_tree_model_get(model, &iter_c, NAME, &name, LEVEL, &level, -1);
				g_string_append_printf(result, " %s (%d)", name, level);
				g_free(name);
			} while (gtk_tree_model_iter_next(model, &iter_c));
			g_string_append_c(result, '\n');
		} while (gtk_tree_model_iter_next(model, &iter_p));
		render_text_nicely(gc, result->str);
	}
	save_to_png(gc, fname);
	destroy_context(gc);
	g_string_chunk_free(pool);
}

int main (int argc, char *argv[]) {
	GError *err = NULL;
	GtkBuilder *builder; // yadda yadda, globals are bad.
	GtkWindow *window;

	gtk_init(&argc, &argv);

	builder = gtk_builder_new();
	gtk_builder_add_from_file(builder, "proj.glade", &err);
	if (err != NULL) {
		puts(err->message);
	}

	window = GTK_WINDOW(gtk_builder_get_object (builder, "window"));


	{
		gtk_file_filter_set_name(
				GTK_FILE_FILTER(gtk_builder_get_object(builder, "filefilter")),
				"Файлы PNG (*.png)"
		);
	}

	skills_init(builder);
	strife_init(builder);

	gtk_builder_connect_signals(builder, builder);

	g_signal_connect(window, "destroy", G_CALLBACK (gtk_main_quit), NULL);
	gtk_widget_show(GTK_WIDGET(window));
	gtk_main();

	return 0;
}
