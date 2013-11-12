#include <gtk/gtk.h>
#include <stdlib.h>
#include "render.h"

enum { FONTWEIGHT = 0, IS_SKILL = 1, NAME = 2, LEVEL = 3 };
enum { DISPLAY = 0, SHORT = 1 };

G_MODULE_EXPORT
void on_window_destroy (GObject *object, gpointer user_data) {
        gtk_main_quit();
}

const gchar *meta_skills[] = {
	"Физические навыки",
	"Ментальные навыки",
	"Социальные навыки",
};

const int a6_width = 827, a6_height = 1165; // 148x105mm, 200dpi
const int margin_v = 100, margin_h = 125; // 0.5", 0.625", 200dpi

const gint damage_multipliers[] = {0, 1, 2, 4, 5, 6, 7, 8};
const gint damage_denominator = 4;

G_MODULE_EXPORT
void on_skill_remove(GtkToolItem *tool, gpointer udata) {
	GtkTreeView *tv = GTK_TREE_VIEW((GObject *)udata);
	GtkTreePath *path = NULL;
	GtkTreeViewColumn *col = NULL;
	GtkTreeStore *store;
	GtkTreeIter iter;
	GtkTreeIter candidate;
	gboolean found = TRUE;
	gint depth;

	gtk_tree_view_get_cursor(tv, &path, &col);
	if (path == NULL) return;
	store = GTK_TREE_STORE(gtk_tree_view_get_model(tv));

	depth = gtk_tree_path_get_depth(path);
	if (depth < 2) return;
	gtk_tree_model_get_iter(GTK_TREE_MODEL(store),
			&iter, path);
	candidate = iter;
	/* Trying to find a candidate if going-to-next fails */
	if (!gtk_tree_model_iter_previous(GTK_TREE_MODEL(store), &candidate)) {
		if (!gtk_tree_model_iter_parent(GTK_TREE_MODEL(store), &candidate, &iter)) {
			found = FALSE;
		}
	}

	if (gtk_tree_store_remove(store, &iter)) {
		candidate = iter;
		found = TRUE;
	}
	if (!found) {
		found = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(store), &candidate);
	}
	gtk_tree_path_free(path);
	if (found) {
		path = gtk_tree_model_get_path(GTK_TREE_MODEL(store), &candidate);
		gtk_tree_view_set_cursor(tv, path, col, FALSE);
		gtk_tree_path_free(path);
	}

	printf("remove skill %p\n", tv);
}

G_MODULE_EXPORT
void on_skill_add(GtkToolItem *tool, gpointer udata) {
	GtkTreeView *tv = GTK_TREE_VIEW((GObject *)udata);
	GtkTreePath *path = NULL;
	GtkTreeViewColumn *col = NULL;
	GtkTreeStore *store;
	GtkTreeIter i1, i2;
	gint depth;

	gtk_tree_view_get_cursor(tv, &path, &col);
	if (path == NULL) return;
	store = GTK_TREE_STORE(gtk_tree_view_get_model(tv));

	depth = gtk_tree_path_get_depth(path);
	if (depth < 1) return;
	if (depth > 1) { gtk_tree_path_up(path); }
	gtk_tree_model_get_iter(GTK_TREE_MODEL(store),
			&i1, path);
	gtk_tree_path_free(path);

	gtk_tree_store_append(store, &i2, &i1);
	gtk_tree_store_set(store, &i2,
			FONTWEIGHT, 400,
			IS_SKILL, TRUE,
			NAME, "",
			LEVEL, (gint) 1,
			-1);

	path = gtk_tree_model_get_path(GTK_TREE_MODEL(store), &i2);
	col = gtk_tree_view_get_column(tv, 0);
	gtk_tree_view_expand_to_path(tv, path);
	gtk_tree_view_set_cursor(tv, path, col, TRUE);
	gtk_tree_path_free(path);

	printf("add skill %p\n", tv);
}

G_MODULE_EXPORT
void render_level(GtkTreeViewColumn *col,
		GtkCellRenderer *cr,
		GtkTreeModel *model,
		GtkTreeIter *iter, gpointer _) {
	gint value;
	gchar *text;
	gtk_tree_model_get(model, iter, LEVEL, &value, -1);
	text = g_strdup_printf("%d", value);
	g_object_set(cr, "text", text, NULL);
	g_free(text);
}

G_MODULE_EXPORT
void on_name_edited(GtkCellRendererText *obj, gchar *cpath, gchar *new_name,
		gpointer udata) {
	GtkTreeStore *store = GTK_TREE_STORE((GObject *)udata);
	GtkTreePath *path = gtk_tree_path_new_from_string(cpath);
	GtkTreeIter iter;
	gtk_tree_model_get_iter(GTK_TREE_MODEL(store), &iter, path);
	gtk_tree_store_set(store, &iter,
			NAME, new_name,
			-1);
	gtk_tree_path_free(path);
}

G_MODULE_EXPORT
void on_level_edited(GtkCellRendererSpin *obj, gchar *cpath, gchar *new_level,
		gpointer udata) {
	GtkTreeStore *store = (GtkTreeStore *)udata;
	GtkTreePath *path = gtk_tree_path_new_from_string(cpath);
	GtkAdjustment *adj;
	GtkTreeIter iter;
	gdouble value;
	gchar *errptr;

	printf("obj passed: %p\n", obj);
	g_object_get(obj, "adjustment", &adj, NULL);
	value = (gdouble)strtol(new_level, &errptr, 0);
	if (!*errptr && *cpath) {
		gtk_adjustment_set_value(adj, value);
	} else {
		value = gtk_adjustment_get_value(adj);
	}

	gtk_tree_model_get_iter(GTK_TREE_MODEL(store), &iter, path);
	gtk_tree_store_set(store, &iter,
			LEVEL, (gint) value,
			-1);
	gtk_tree_path_free(path);
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


G_MODULE_EXPORT
void invoke_print(GObject *stupid_button, GtkBuilder *builder) {
	GString *result = g_string_new("");
	GStringChunk *pool = g_string_chunk_new(1024);
	GC *gc = create_context(a6_width, a6_height, 200);

	draw_nice_progression(gc, margin_h, 50);
	tmargin(gc, margin_v); bmargin(gc, margin_v);
	lmargin(gc, margin_h);
	draw_bubble_progression(gc, 10, 50);

	{
		g_string_printf(result, "%s, %s %d уровня\n",
				entry_text(builder, "ent_name"),
				entry_text(builder, "ent_class"),
				spin_value(builder, "spin_level"));
		g_string_append_printf(result, "ЗД: \t%d\nУст: \t%d\nОД: \t%d\n",
				spin_value(builder, "spin_hp"),
				spin_value(builder, "spin_fgue"),
				spin_value(builder, "spin_ap"));
//		fputs(result->str, stdout);
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
		//fputs(result->str, stdout);
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
		//fputs(result->str, stdout);
		render_text_nicely(gc, result->str);
	}
	save_to_png(gc, "test.png");
	destroy_context(gc);
	g_string_chunk_free(pool);
}

int main (int argc, char *argv[]) {
	GError *err = NULL;
	GtkBuilder *builder; // yadda yadda, globals are bad.
	GtkWidget *window;
	GtkTreeStore *store;
	GtkTreeIter iter;
	int i;

	gtk_init(&argc, &argv);

	builder = gtk_builder_new();
	gtk_builder_add_from_file(builder, "proj.glade", &err);
	if (err != NULL) {
		puts(err->message);
	}

	window = GTK_WIDGET(gtk_builder_get_object (builder, "window"));
	store = GTK_TREE_STORE(gtk_builder_get_object(builder, "treestore_skills"));

	gtk_tree_store_clear(store);
	for (i = 0; i < 3; i++) {
		gtk_tree_store_append(store, &iter, NULL);
		gtk_tree_store_set(store, &iter,
				FONTWEIGHT, 700,
				IS_SKILL, FALSE,
				NAME, meta_skills[i],
				LEVEL, (gint) 0,
				-1);
	}



	{
		GtkTreeViewColumn *col = GTK_TREE_VIEW_COLUMN(gtk_builder_get_object(builder, "tvskills_level"));
		GtkCellRenderer *cr = GTK_CELL_RENDERER(gtk_builder_get_object(builder, "tvskills_cr_level"));
		gtk_toolbar_set_icon_size(GTK_TOOLBAR(gtk_builder_get_object(builder, "toolbar_skills")),
				GTK_ICON_SIZE_MENU);
		gtk_tree_view_column_set_cell_data_func(col, cr, render_level, NULL, NULL);
	}

	gtk_builder_connect_signals(builder, builder);

	g_signal_connect(window, "destroy", G_CALLBACK (gtk_main_quit), NULL);
	gtk_widget_show(window);
	gtk_main();

	return 0;
}
