#ifndef STRIFE_H
#define STRIFE_H
#include <gtk/gtk.h>
enum { STRIFE_NAME = 0, STRIFE_VALUE = 1, STRIFE_TYPE = 2 };
enum { WPN_USE_SKILL = 5, WPN_MASTERY_SKILL = 4, WPN_MASTERY = 3 };
enum { FILTER_NAME = 0, FILTER_LVL = 1 };

extern void strife_init(GtkBuilder *builder);
#endif
