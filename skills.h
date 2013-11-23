#ifndef SKILLS_H
#define SKILLS_H
#include <gtk/gtk.h>

enum { FONTWEIGHT = 0, IS_SKILL = 1, NAME = 2, LEVEL = 3 };
enum { DISPLAY = 0, SHORT = 1 };

extern const gchar *meta_skills[3];

extern void skills_init(GtkBuilder *builder);

#endif
