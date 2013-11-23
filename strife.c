#include "strife.h"
#include <stdlib.h>
#include <string.h>
#include <skills.h>

GType filter_types[] = {G_TYPE_STRING, G_TYPE_INT};
static const char *LAST_SELECTED = "last-selected";

G_MODULE_EXPORT
void add_new_strife(GtkEntry *e, GtkEntryIconPosition icon_pos, GdkEvent *event,
	GtkListStore *store) {
	GtkTreeIter iter;
	GtkComboBox *box;
	if (event->type != GDK_BUTTON_PRESS || event->button.button != 1) {
		return;
	}
	gtk_list_store_insert_with_values(store, &iter, -1,
		STRIFE_NAME, "Добавлено", STRIFE_VALUE, 0, STRIFE_TYPE, 0, -1);
	box = GTK_COMBO_BOX(gtk_widget_get_parent(GTK_WIDGET(e)));
	gtk_combo_box_set_active_iter(box, &iter);
	gtk_widget_set_can_focus(GTK_WIDGET(e), TRUE);
	gtk_widget_grab_focus(GTK_WIDGET(e));
	gtk_editable_select_region(GTK_EDITABLE(e), 0, -1);
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
	g_object_set_data_full(G_OBJECT(box), LAST_SELECTED,
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

G_MODULE_EXPORT
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

G_MODULE_EXPORT
void update_strife(GtkComboBox *box, GtkWidget *w) {
	GtkListStore *ls = GTK_LIST_STORE(gtk_combo_box_get_model(box));
	gchar *path = g_object_get_data(G_OBJECT(box), LAST_SELECTED);
	GValue value = G_VALUE_INIT;
	GtkTreeIter it;
	const gchar *wname = gtk_buildable_get_name(GTK_BUILDABLE(w));
	gint column = -1;
	if (!path || !gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(ls), &it, path))
		return;
	if (match("combo_entry_......", wname)) {
		// special case; delete if empty
		const gchar *text = gtk_entry_get_text(GTK_ENTRY(w));
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

G_MODULE_EXPORT
void possibly_delete(GtkEntry *e, GtkComboBox *box) {
	const gchar *text = gtk_entry_get_text(e);
	GtkListStore *ls = GTK_LIST_STORE(gtk_combo_box_get_model(box));
	gchar *path = g_object_get_data(G_OBJECT(box), LAST_SELECTED);
	GtkTreeIter it;
	if (!path || !gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(ls), &it, path))
		return;
	if (!text || !*text) {
		if (gtk_list_store_remove(ls, &it)) {
			gtk_combo_box_set_active_iter(box, &it);
		} else {
			gtk_combo_box_set_active_iter(box, NULL);
		}
		return;
	}
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

void strife_init(GtkBuilder *builder) {
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
