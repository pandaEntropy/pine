#include "wm.h"
#include "layout.h"

#include <X11/Xlib.h>
#include <stdio.h>
#include <stdbool.h>

MasterPosition master_pos = MASTER_LEFT;
float mfactor = 0.5;
int nmaster = 1;
int ntiled = 0;

int border_width = 4;

typedef struct{
    int width;
    int height;
    int x;
    int y;
}Rect;

void moveresize_window(WM *wm, Client *c, unsigned int width, unsigned int height, int x, int y);

Layout master_layout(){
    return (Layout){.id = LAYOUT_MASTER, .tile = master_tile, .rotate = master_rotate, .focus = focus_direction};
}

Layout monocle_layout(){
    return (Layout){.id = LAYOUT_MONOCLE, .tile = monocle_tile, .rotate = monocle_rotate, .focus = monocle_focus};
}

void tile(WM *wm){
    ntiled = 0;

    for(Client *c = wm->clients; c; c = c->next){
        if(!c->floating)
            ntiled++;
    }

    wm->layouts[wm->active_layout].tile(wm);
}

void split_horizontal(Rect area, float factor, Rect *a, Rect *b){
    int split = area.height * factor;

    *a = (Rect){area.width, split, area.x, area.y};
    *b = (Rect){area.width, area.height - split, area.x, area.y + split};
}

void split_vertical(Rect area, float factor, Rect *a, Rect *b){
    int split = area.width * factor;

    *a = (Rect){split, area.height, area.x, area.y};
    *b = (Rect){area.width - split, area.height, area.x + split, area.y};
}

void stack_rects(bool horizontal, Rect area, int nrects, Rect *stack){
    for(int i = 0; i < nrects; i++){
        if(horizontal){ //horizontal stacking
            int h = area.height / nrects;

            stack[i] = (Rect){
                area.width,
                (i == nrects-1) ? area.height - i * h : h,
                area.x,
                area.y + i * h
            };
        }
        else{
            int w = area.width / nrects;

            stack[i] = (Rect){
                (i == nrects-1) ? area.width - i * w : w,
                area.height,
                area.x + i * w,
                area.y
            };
 
        }
    }
}

void master_tile(WM *wm){
    if(ntiled == 0 || nmaster == 0) return;

    int nstack = ntiled - nmaster;
    if(nstack <= 0){
        moveresize_window(wm, wm->master, wm->usable_width, wm->usable_height, wm->usable_x, wm->usable_y);
        return;
    }

    Rect root = {wm->usable_width, wm->usable_height, wm->usable_x, wm->usable_y};

    Rect master_area, stack_area;

    switch(master_pos){
        case MASTER_LEFT:
            split_vertical(root, mfactor, &master_area, &stack_area);
            break;

        case MASTER_RIGHT:
            split_vertical(root, 1 - mfactor, &stack_area, &master_area);
            break;

        case MASTER_TOP:
            split_horizontal(root, mfactor, &master_area, &stack_area);
            break;

        case MASTER_BOTTOM:
            split_horizontal(root, 1 - mfactor, &stack_area, &master_area);
            break;
    }

    Rect stacked_rects[ntiled - nmaster];
    stack_rects(master_pos == MASTER_RIGHT || master_pos == MASTER_LEFT, stack_area, nstack, stacked_rects);

    moveresize_window(wm, wm->master, master_area.width, master_area.height, master_area.x, master_area.y);

    int i = 0;
    for(Client *c = wm->clients; c; c = c->next){
        if(c->floating || wm->master == c)
            continue;

        Rect r = stacked_rects[i];

        moveresize_window(wm, c, r.width, r.height, r.x, r.y);
        i++;
    }

}

void monocle_tile(WM *wm){
    if(!wm->clients) return;

    for(Client *c = wm->clients; c; c = c->next){
        if(c->floating) continue;

        moveresize_window(wm, c, wm->usable_width, wm->usable_height, wm->usable_x, wm->usable_y);
    }

    if(wm->focused){
        XRaiseWindow(wm->dpy, wm->focused->parent);
        XRaiseWindow(wm->dpy, wm->focused->win);
    }
}

void resize(WM *wm, const Arg *arg){
    if(wm->layouts[wm->active_layout].id != LAYOUT_MASTER) return;

    int dir = arg->i;
    float change = 0;

    if(ntiled < 2)
        return;
    switch(master_pos){
        case MASTER_LEFT:
            if(dir == DIR_LEFT) change = -0.05;
            if(dir == DIR_RIGHT) change = 0.05;
            break;

        case MASTER_RIGHT:
            if(dir == DIR_LEFT) change = 0.05;
            if(dir == DIR_RIGHT) change = -0.05;
            break;

        case MASTER_TOP:
            if(dir == DIR_UP) change = -0.05;
            if(dir == DIR_DOWN) change = 0.05;
            break;

        case MASTER_BOTTOM:
            if(dir == DIR_UP) change = 0.05;
            if(dir == DIR_DOWN) change = -0.05;
            break;
    }
    if(mfactor + change > 0.95 || mfactor + change < 0.05)
        return;

    mfactor += change;
    tile(wm);
}

void rotate(WM *wm, const Arg *arg){
    (void)arg;
    if(ntiled < 2) return;

    wm->layouts[wm->active_layout].rotate(wm);
}

void master_rotate(WM *wm){
    //Cycle the enum
    master_pos = (master_pos+1) % 4;
    tile(wm);
}

void monocle_rotate(WM *wm){
    (void)wm;
}

void parent_center(WM *wm, Window parent, Client *c){
    XWindowAttributes pattr;
    XWindowAttributes cattr;

    XGetWindowAttributes(wm->dpy, parent, &pattr);
    XGetWindowAttributes(wm->dpy, c->win, &cattr);

    int x = (pattr.width - cattr.width) / 2;
    int y = (pattr.height - cattr.height) / 2;

    if(x < 0) x = 0;
    if(y < 0) y = 0;

    moveresize_window(wm, c, cattr.width, cattr.height, x, y);
}

void screen_center(WM *wm, Client *c){
    XWindowAttributes cattr;

    XGetWindowAttributes(wm->dpy, c->win, &cattr);

    int x = (wm->usable_width - cattr.width) / 2;
    int y = (wm->usable_height - cattr.height) / 2;

    if(x < 0) x = 0;
    if(y < 0) y = 0;

    moveresize_window(wm, c, cattr.width, cattr.height, x, y);
}

void moveresize_window(WM *wm, Client *c, unsigned int width, unsigned int height, int x, int y){
    if(c->parent){
        XMoveResizeWindow(wm->dpy, c->parent, x, y, width - 2 * border_width, height - 2 * border_width);
        XMoveResizeWindow(wm->dpy, c->win, 0, 0, width - 2 * border_width, height - 2 * border_width);
    }
    else
        XMoveResizeWindow(wm->dpy, c->win, x, y, width, height);


    send_conf_req(wm, c, width, height, x, y);
}
