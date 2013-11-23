#include "skills.h"
#include <stdlib.h>

const gchar *meta_skills[] = {
	"Физические навыки",
	"Ментальные навыки",
	"Социальные навыки",
};

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

void skills_init(GtkBuilder *builder) {
	int i;
	GtkTreeStore *store = GTK_TREE_STORE(gtk_builder_get_object(builder, "treestore_skills"));
	GtkTreeViewColumn *col = GTK_TREE_VIEW_COLUMN(gtk_builder_get_object(builder, "tvskills_level"));
	GtkCellRenderer *cr = GTK_CELL_RENDERER(gtk_builder_get_object(builder, "tvskills_cr_level"));
	GtkTreeIter iter;

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

	gtk_toolbar_set_icon_size(GTK_TOOLBAR(gtk_builder_get_object(builder, "toolbar_skills")),
			GTK_ICON_SIZE_MENU);
	gtk_tree_view_column_set_cell_data_func(col, cr, render_level, NULL, NULL);
}
