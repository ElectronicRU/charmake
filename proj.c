#include "schema.h"
#include <gtk/gtk.h>

enum { IS_META = 0, IS_SKILL = 1, NAME = 2, LEVEL = 3 };

void 
on_window_destroy (GObject *object, gpointer user_data)
{
        gtk_main_quit();
}

char *meta_skills[] = {
	"<b>Физические навыки</b>",
	"<b>Ментальные навыки</b>",
	"<b>Социальные навыки</b>",
};

void on_skill_remove(GtkAction *action, gpointer udata) {
	GtkTreeView *tv = GTK_TREE_VIEW((GObject *)udata);
	GtkTreePath *path = NULL;
	GtkTreeViewColumn *col = NULL;
	GtkTreeStore *store;
	GtkTreeIter iter;
	gint depth;

	gtk_tree_view_get_cursor(tv, &path, &col);
	store = GTK_TREE_STORE(gtk_tree_view_get_model(tv));

	depth = gtk_tree_path_get_depth(path);
	if (depth < 2) return;
	gtk_tree_model_get_iter(GTK_TREE_MODEL(store),
			&iter, path);
	if (gtk_tree_store_remove(store, &iter)) {
		gtk_tree_path_free(path);
		path = gtk_tree_model_get_path(GTK_TREE_MODEL(store), &iter);
	} else {
		gtk_tree_path_up(path);
		gtk_tree_path_down(path);
	}
	gtk_tree_view_set_cursor(tv, path, col, FALSE);
	gtk_tree_path_free(path);

	printf("remove skill %p\n", tv);
}

void on_skill_add(GtkAction *action, gpointer udata) {
	GtkTreeView *tv = GTK_TREE_VIEW((GObject *)udata);
	GtkTreePath *path = NULL;
	GtkTreeViewColumn *col = NULL;
	GtkTreeStore *store;
	GtkTreeIter i1, i2;
	gint depth;

	gtk_tree_view_get_cursor(tv, &path, &col);
	store = GTK_TREE_STORE(gtk_tree_view_get_model(tv));

	depth = gtk_tree_path_get_depth(path);
	if (depth < 1) return;
	if (depth > 1) { gtk_tree_path_up(path); }
	gtk_tree_model_get_iter(GTK_TREE_MODEL(store),
			&i1, path);
	gtk_tree_path_free(path);

	gtk_tree_store_append(store, &i2, &i1);
	gtk_tree_store_set(store, &i2,
			IS_META, FALSE,
			IS_SKILL, TRUE,
			NAME, g_strdup(""),
			LEVEL, (gint) 1,
			-1);

	path = gtk_tree_model_get_path(GTK_TREE_MODEL(store), &i2);
	col = gtk_tree_view_get_column(tv, 0);
	gtk_tree_view_expand_to_path(tv, path);
	gtk_tree_view_set_cursor(tv, path, col, TRUE);
	gtk_tree_path_free(path);

	printf("add skill %p\n", tv);
}

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

void on_level_edited(GtkCellRendererText *obj, gchar *cpath, gchar *new_level,
		gpointer udata) {
	GtkTreeStore *store = GTK_TREE_STORE((GObject *)udata);
	GtkTreePath *path = gtk_tree_path_new_from_string(cpath);
	GtkAdjustment *adj;
	GtkTreeIter iter;

	g_object_get(obj, "adjustment", &adj, NULL);
	printf("level set %s\n", new_level);

	gtk_tree_model_get_iter(GTK_TREE_MODEL(store), &iter, path);
	gtk_tree_store_set(store, &iter,
			LEVEL, (gint) gtk_adjustment_get_value(adj),
			-1);
	gtk_tree_path_free(path);
}

int
main (int argc, char *argv[])
{
        GtkBuilder              *builder;
        GtkWidget               *window;
		GtkTreeStore *store;
		GtkTreeIter iter;
		int i;
        
        gtk_init (&argc, &argv);
        
        builder = gtk_builder_new ();
    //    gtk_builder_add_from_file (builder, "proj.glade", NULL);
		gtk_builder_add_from_string(builder, schema, sizeof(schema), NULL);

        window = GTK_WIDGET (gtk_builder_get_object (builder, "window"));
		store = GTK_TREE_STORE(gtk_builder_get_object(builder, "treestore_skills"));

		for (i = 0; i < 3; i++) {
			gtk_tree_store_append(store, &iter, NULL);
			gtk_tree_store_set(store, &iter,
					IS_META, TRUE,
					IS_SKILL, FALSE,
					NAME, g_strdup(meta_skills[i]),
					LEVEL, (gint) 0,
					-1);
		}

		{
			GtkTreeViewColumn *col = GTK_TREE_VIEW_COLUMN(gtk_builder_get_object(builder, "tvskills_level"));
			GtkCellRenderer *cr = GTK_CELL_RENDERER(gtk_builder_get_object(builder, "tvskills_cr_level"));
			gtk_tree_view_column_set_cell_data_func(col, cr, render_level, NULL, NULL);
		}

        gtk_builder_connect_signals (builder, NULL);          
        g_object_unref (G_OBJECT (builder));
        
		g_signal_connect (window, "destroy", G_CALLBACK (gtk_main_quit), NULL);
        gtk_widget_show (window);       
        gtk_main ();
        
        return 0;
}