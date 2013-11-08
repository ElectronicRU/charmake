#include <glib.h>
#include <stdio.h>

#define BUFSIZE 1024
int main(void) {
	gchar buf[BUFSIZE];
	FILE *f1, *f2;
	f1 = fopen("proj.glade", "r");
	f2 = fopen("schema.h", "w");
	fputs("const char schema[] = \"\\\n", f2);
	while (1) {
		gchar *dup;
		fgets(buf, BUFSIZE, f1);
		if (feof(f1)) break;
		dup = g_strescape(buf, "");
		fputs(dup, f2);
		fputs("\\\n", f2);
		g_free(dup);
	}
	fclose(f1);
	fputs("\";\n", f2);
	fclose(f2);
}
