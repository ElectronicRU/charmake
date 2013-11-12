#ifndef RENDER_H
#define RENDER_H
#include <stdbool.h>
struct GC;
typedef struct GC GC;
GC *create_context(int, int, int);
void lmargin(GC *, double);
void rmargin(GC *, double);
void tmargin(GC *, double);
void bmargin(GC *, double);
void draw_nice_progression(GC *gc, double marginsize, int n);
void draw_bubble_progression(GC *gc, int number, int multiply);
void render_text_nicely(GC *gc, const char *text);
void save_to_png(GC *gc, const char *filename);
void destroy_context(GC *gc);

#endif
