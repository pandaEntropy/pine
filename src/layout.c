#include "pine.h"
#include "layout.h"

#include <X11/Xlib.h>
#include <stdbool.h>

#define MAX_MASTER 1

int ntiled = 0;

typedef struct{
    int width;
    int height;
    int x;
    int y;
}Rect;

typedef struct LayoutTarget{
    Client *client; Rect geom; }LayoutTarget;

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
        if(c->wtags & wm->current_wtag){
            if(!c->floating){
                ntiled++;
            }
        }
        else{
            XUnmapWindow(wm->dpy, c->win);
        }
    }

    if(ntiled > 0){
        LayoutTarget targets[ntiled];
        wm->workspaces[wm->current_ws].layout->tile(wm, targets);

        int i = 0;
        for(Client *c = wm->clients; c; c = c->next){
            if(!c->floating && (c->wtags & wm->current_wtag)){
                targets[i].client = c;
                i++;
            }
        }

        for(int j = 0; j < ntiled; j++){
            moveresize_window(wm, targets[j].client,
                    targets[j].geom.width,
                    targets[j].geom.height,
                    targets[j].geom.x,
                    targets[j].geom.y);
        }
    }

    for(Client *c = wm->clients; c; c = c->next){
        if(c->wtags & wm->current_wtag){
            XMapWindow(wm->dpy, c->win);
        }
    }
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

void master_tile(WM *wm, LayoutTarget *targets){
    if(ntiled == 0) return;

    int nstack = ntiled - MAX_MASTER; //the max master count and is always 1
    if(nstack <= 0){
        targets[0].geom = (Rect){wm->usable_width, wm->usable_height, wm->usable_x, wm->usable_y};
        return;
    }

    Rect root = {wm->usable_width, wm->usable_height, wm->usable_x, wm->usable_y};

    Rect master_area, stack_area;

    Workspace *ws = &wm->workspaces[wm->current_ws];

    switch(ws->master_pos){
        case MASTER_LEFT:
            split_vertical(root, ws->mfactor, &master_area, &stack_area);
            break;

        case MASTER_RIGHT:
            split_vertical(root, 1 - ws->mfactor, &stack_area, &master_area);
            break;

        case MASTER_TOP:
            split_horizontal(root, ws->mfactor, &master_area, &stack_area);
            break;

        case MASTER_BOTTOM:
            split_horizontal(root, 1 - ws->mfactor, &stack_area, &master_area);
            break;
    }

    Rect stacked_rects[ntiled - MAX_MASTER];
    stack_rects(ws->master_pos == MASTER_RIGHT || ws->master_pos == MASTER_LEFT, stack_area, nstack, stacked_rects);

    targets[0].geom = master_area;

    for(int i = MAX_MASTER; i < ntiled; i++){
        targets[i].geom = stacked_rects[i - MAX_MASTER];
    }
}

void monocle_tile(WM *wm, LayoutTarget *targets){
    for(int i = 0; i < ntiled; i++){
        targets[i].geom = (Rect){wm->usable_width, wm->usable_height, wm->usable_x, wm->usable_y};
    }
}

void resize(WM *wm, int dir){
    if(wm->workspaces[wm->current_ws].layout->id != LAYOUT_MASTER) return;

    float change = 0;

    if(ntiled < 2)
        return;

    Workspace *ws = &wm->workspaces[wm->current_ws];
    switch(ws->master_pos){
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
    if(ws->mfactor + change > 0.95 || ws->mfactor + change < 0.05)
        return;

    ws->mfactor += change;
    tile(wm);
}

void master_rotate(WM *wm){
    if(ntiled < 2)
        return;

    wm->workspaces[wm->current_ws].master_pos = (wm->workspaces[wm->current_ws].master_pos+1) % 4;
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

    int x = pattr.x + (pattr.width - cattr.width) / 2;
    int y = pattr.y + (pattr.height - cattr.height) / 2;

    if(x < 0) x = 0;
    if(y < 0) y = 0;

    if(c->wtags & wm->current_wtag){
        moveresize_window(wm, c, cattr.width, cattr.height, x, y);
        XMapWindow(wm->dpy, c->win);
    }
}

void screen_center(WM *wm, Client *c){
    XWindowAttributes cattr;

    XGetWindowAttributes(wm->dpy, c->win, &cattr);

    int x = (wm->usable_width - cattr.width) / 2;
    int y = (wm->usable_height - cattr.height) / 2;

    if(x < 0) x = 0;
    if(y < 0) y = 0;

    if(c->wtags & wm->current_wtag){
        moveresize_window(wm, c, cattr.width, cattr.height, x, y);
    }

    XMapWindow(wm->dpy, c->win);
}

void moveresize_window(WM *wm, Client *c, unsigned int width, unsigned int height, int x, int y){
    int border_width = wm->config.border_width;

    XMoveResizeWindow(wm->dpy, c->win, x, y, width - 2 * border_width, height - 2 * border_width);

    send_conf_req(wm, c, width, height, x, y);
}
