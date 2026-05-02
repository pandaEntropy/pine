#ifndef LAYOUT_H
#define LAYOUT_H

#include "forward.h"
#include <X11/Xlib.h>

typedef enum{
    MASTER_LEFT,
    MASTER_TOP,
    MASTER_RIGHT,
    MASTER_BOTTOM
} MasterPosition;

Layout master_layout();

Layout horizontal_layout();

Layout monocle_layout();

void tile(WM *wm);

void master_tile(WM *wm, LayoutTarget *targets);

void monocle_tile(WM *wm, LayoutTarget *targets);

void master_rotate(WM *wm);

void monocle_rotate(WM *wm);

void horizontal_rotate(WM *wm);

void resize(WM *wm, int dir);

void parent_center(WM *wm, Window parent, Client *c);

void screen_center(WM *wm, Client *c);

#endif
