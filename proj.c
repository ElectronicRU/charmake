#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>
#include "render.h"

GType filter_types[] = {G_TYPE_STRING, G_TYPE_INT};
enum { FONTWEIGHT = 0, IS_SKILL = 1, NAME = 2, LEVEL = 3 };
enum { DISPLAY = 0, SHORT = 1 };
enum { STRIFE_NAME = 0, STRIFE_VALUE = 1, STRIFE_TYPE = 2 };
enum { WPN_USE_SKILL = 5, WPN_MASTERY_SKILL = 4, WPN_MASTERY = 3 };
enum { FILTER_NAME = 0, FILTER_LVL = 1 };


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

	gtk_tree_store_insert_with_values(store, &i2, &i1, -1,
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
	GtkAdjustment *adj;
	GtkTreeIter iter;
	gdouble value;
	gchar *errptr;

	g_object_get(obj, "adjustment", &adj, NULL);
	value = (gdouble)strtol(new_level, &errptr, 0);
	if (!*errptr && *cpath) {
		gtk_adjustment_set_value(adj, value);
	} else {
		value = gtk_adjustment_get_value(adj);
	}

	gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(store), &iter, cpath);
	gtk_tree_store_set(store, &iter,
			LEVEL, (gint) value,
			-1);
}

G_MODULE_EXPORT
void add_new_strife(GtkEntry *e, GtkEntryIconPosition icon_pos, GdkEvent *event,
	GtkListStore *store) {
	GtkTreeIter iter;
	GtkComboBox *box;
	if (event->type != GDK_BUTTON_PRESS || event->button.button != 1) {
		return;
	}
	gtk_list_store_insert_with_values(store, &iter, -1,
		STRIFE_NAME, "", STRIFE_VALUE, 0, STRIFE_TYPE, 0, -1);
	box = GTK_COMBO_BOX(gtk_widget_get_parent(GTK_WIDGET(e)));
	gtk_combo_box_set_active_iter(box, &iter);
	gtk_widget_set_can_focus(GTK_WIDGET(e), TRUE);
	gtk_widget_grab_focus(GTK_WIDGET(e));
}

G_MODULE_EXPORT
void change_strife(GtkComboBox *box, GtkGrid *grid) {
	GtkTreeModel *ls = gtk_combo_box_get_model(box);
	GtkTreeIter i;
	gboolean nonempty = gtk_tree_model_get_iter_first(ls, &i);
	GtkWidget *internal = gtk_bin_get_child(GTK_BIN(box));
	gtk_widget_set_sensitive(GTK_WIDGET(grid), nonempty);
	gtk_editable_set_editable(GTK_EDITABLE(internal), nonempty);
	gtk_widget_set_can_focus(internal, nonempty);
	if (!gtk_combo_box_get_active_iter(box, &i)) {
		// just some text typed
		return;
	}
	g_object_set_data_full(G_OBJECT(box), "last-selected",
			gtk_tree_model_get_string_from_iter(ls, &i),
			g_free);
	{
		gint damage;
		gchar *dmtype;
		GtkSpinButton *btn = GTK_SPIN_BUTTON(
				gtk_grid_get_child_at(grid, 3, 0));
		GtkComboBox *c = GTK_COMBO_BOX(
				gtk_grid_get_child_at(grid, 0, 0));
		gtk_tree_model_get(ls, &i, STRIFE_VALUE, &damage, STRIFE_TYPE, &dmtype, -1);
		gtk_spin_button_set_value(btn, (gdouble)damage);
		gtk_combo_box_set_active_id(c, dmtype);
		g_free(dmtype);
	}
	if (gtk_tree_model_get_n_columns(ls) > 3) {
		gboolean use_skill;
		gint skill_lvl;
		gchar *skill;
		GtkToggleButton *c = GTK_TOGGLE_BUTTON(
				gtk_grid_get_child_at(grid, 0, 1));
		GtkComboBox *sb = GTK_COMBO_BOX(
				gtk_grid_get_child_at(grid, 1, 1));
		GtkSpinButton *lsp = GTK_SPIN_BUTTON(
				gtk_grid_get_child_at(grid, 3, 1));
		gtk_tree_model_get(ls, &i, WPN_USE_SKILL, &use_skill,
				WPN_MASTERY_SKILL, &skill, WPN_MASTERY, &skill_lvl, -1);
		gtk_spin_button_set_value(lsp, (gdouble)skill_lvl);
		use_skill = gtk_combo_box_set_active_id(sb, skill) && use_skill;
		// notify toggle button, since its handler corrects actual values
		// and sensitivities
		g_object_set(G_OBJECT(c), "active", use_skill, NULL);
		g_free(skill);
	}
}

void show_strife_mastery(GtkToggleButton *btn, GParamSpec *spec, GtkGrid *grid) {
	gboolean active = gtk_toggle_button_get_active(btn);
	GtkSpinButton *spinlvl = GTK_SPIN_BUTTON(gtk_grid_get_child_at(grid, 3, 1));
	GtkComboBox *skillcombo = GTK_COMBO_BOX(
			gtk_grid_get_child_at(grid, 1, 1));
	GtkTreeModel *tm = gtk_combo_box_get_model(skillcombo);
	GtkTreeIter it;
	gboolean can_be_active = gtk_tree_model_get_iter_first(tm, &it); // basically, "nonempty"

	if (!can_be_active) {
		active = FALSE;
		gtk_toggle_button_set_active(btn, FALSE);
	}
	gtk_editable_set_editable(GTK_EDITABLE(spinlvl), !active);
	gtk_widget_set_sensitive(GTK_WIDGET(skillcombo), active);
	gtk_widget_set_sensitive(GTK_WIDGET(btn), can_be_active);
	
	if (active) {
		if (gtk_combo_box_get_active_iter(skillcombo, &it)) {
			gint lvl;
			gtk_tree_model_get(tm, &it, FILTER_LVL, &lvl, -1);
			gtk_spin_button_set_value(spinlvl, (gdouble)lvl);
		}
	}
}

static
gboolean match(const char *pattern, const char *string) {
	do {
		if (*pattern != '.' && *pattern != *string)
			return 0;
	} while (*pattern++ && *string++);
	return 1;
}

void update_strife(GtkComboBox *box, GtkWidget *w) {
	GtkListStore *ls = GTK_LIST_STORE(gtk_combo_box_get_model(box));
	gchar *path = g_object_get_data(G_OBJECT(box), "last-selected");
	GValue value = G_VALUE_INIT;
	GtkTreeIter it;
	const gchar *wname = gtk_buildable_get_name(GTK_BUILDABLE(w));
	gint column = -1;
	if (!path) { // the fuck, should've been nonsensitive
		return;
	}
	gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(ls), &it, path);
	if (match("combo_entry_......", wname)) {
		// special case; delete if empty
		const gchar *text = gtk_entry_get_text(GTK_ENTRY(w));
		if (!text || !*text) {
			if (gtk_list_store_remove(ls, &it)) {
				gtk_combo_box_set_active_iter(box, &it);
			} else {
				gtk_combo_box_set_active_iter(box, NULL);
			}
			return;
		}
		g_value_init(&value, G_TYPE_STRING);
		g_value_set_static_string(&value, text);
		column = STRIFE_NAME;
	} else if (match("spin_..._value", wname)) {
		GtkSpinButton *spval = GTK_SPIN_BUTTON(w);
		gint val = gtk_spin_button_get_value_as_int(spval);
		g_value_init(&value, G_TYPE_INT);
		g_value_set_int(&value, val);
		//column = STRIFE_VALUE;
		//gtk_list_store_set(ls, &it, STRIFE_VALUE, val, -1);
	} else if (match("combo_..._type", wname)) {
		GtkComboBox *tbox = GTK_COMBO_BOX(w);
		const gchar *id = gtk_combo_box_get_active_id(tbox);
		g_value_init(&value, G_TYPE_STRING);
		g_value_set_static_string(&value, id);
		column = STRIFE_TYPE;
	}
	if (column != -1) {
		gtk_list_store_set_value(ls, &it, column, &value);
	}
	g_value_unset(&value);
}

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

char *MASTERY_PREFIX = "Использование оружия: ";

gboolean filter_mastery_skills(GtkTreeModel *model,
		GtkTreeIter *iter,
		gpointer data) {
	gchar *prefix = data;
	gchar *name;
	gboolean res;
	gtk_tree_model_get(model, iter, NAME, &name, -1);
	res = name && (strncmp(prefix, name, strlen(prefix)) == 0);
	g_free(name);
	return res;
}


void show_mastery_skill(GtkTreeModel *m,
		GtkTreeIter *iter,
		GValue *value,
		gint column,
		gpointer data) {
	GtkTreeModelFilter *filter = GTK_TREE_MODEL_FILTER(m);
	char *prefix = data;
	GtkTreeIter citer;
	GtkTreeModel *real = gtk_tree_model_filter_get_model(filter);
	gtk_tree_model_filter_convert_iter_to_child_iter(filter, &citer, iter);
	if (column == FILTER_LVL) {
		g_value_unset(value);
		gtk_tree_model_get_value(real, &citer, LEVEL, value);
	} else {
		gchar *name;
		gtk_tree_model_get(real, &citer, NAME, &name, -1);
		g_value_take_string(value, g_strdup(name + strlen(prefix)));
		g_free(name);
	}
}

int main (int argc, char *argv[]) {
	GError *err = NULL;
	GtkBuilder *builder; // yadda yadda, globals are bad.
	GtkWindow *window;
	GtkTreeStore *store;
	GtkTreeIter iter;
	int i;

	gtk_init(&argc, &argv);

	builder = gtk_builder_new();
	gtk_builder_add_from_file(builder, "proj.glade", &err);
	if (err != NULL) {
		puts(err->message);
	}

	window = GTK_WINDOW(gtk_builder_get_object (builder, "window"));
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

	{
		gtk_file_filter_set_name(
				GTK_FILE_FILTER(gtk_builder_get_object(builder, "filefilter")),
				"Файлы PNG (*.png)"
		);
	}
	{
		GtkComboBox *box = GTK_COMBO_BOX(gtk_builder_get_object(builder, "combo_wpn_mastery"));
		GtkTreePath *path = gtk_tree_path_new_from_indices(0, -1);
		GtkTreeModel *f = gtk_tree_model_filter_new(
			GTK_TREE_MODEL(gtk_builder_get_object(builder, "treestore_skills")),
			path);
		gtk_tree_model_filter_set_visible_func(GTK_TREE_MODEL_FILTER(f),
				filter_mastery_skills,
				MASTERY_PREFIX,
				NULL);
		gtk_tree_model_filter_set_modify_func(GTK_TREE_MODEL_FILTER(f),
				G_N_ELEMENTS(filter_types), filter_types,
				show_mastery_skill,
				MASTERY_PREFIX,
				NULL);
		gtk_tree_path_free(path);
		gtk_combo_box_set_model(box, GTK_TREE_MODEL(f));
	}

	gtk_builder_connect_signals(builder, builder);

	g_signal_connect(window, "destroy", G_CALLBACK (gtk_main_quit), NULL);
	gtk_widget_show(GTK_WIDGET(window));
	gtk_main();

	return 0;
}
