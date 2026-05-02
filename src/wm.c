#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <stdbool.h>
#include <X11/cursorfont.h>
#include <stdio.h> 
#include <limits.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <X11/Xatom.h>

#include "wm.h"
#include "layout.h"
#include "forward.h"

#define MAX_DOCKS 8
#define MAX_WS 9

#define PROTO_DELETE (1 << 0)
#define PROTO_TAKE_FOCUS (1 << 1)

#define Tag(n) (1U << (n))

typedef enum Wintype{
    WIN_DIALOG,
    WIN_SPLASH,
    WIN_MENU,
    WIN_DOCK,
    WIN_NORMAL
}Wintype;

Wintype classify_window(WM *wm, Window win);

void manage_dock(WM *wm, Window win);

void recalc_usable_area(WM *wm);

bool get_strut_partial(WM *wm, Dock *dock);

bool get_strut(WM *wm, Dock *dock);

void unmanage_dock(WM *wm, Window win);

void OnMapRequest(WM *wm, XMapRequestEvent *ev);

void OnConfigureRequest(WM *wm, XConfigureRequestEvent *ev);

void OnDestroyNotify(WM *wm, XDestroyWindowEvent *ev);

void OnPropertyNotify(WM *wm, XPropertyEvent *ev);

void manage(WM *wm, Window w);

void unmanage(WM *wm, Window win);

void handle_client_msg(WM *wm, XClientMessageEvent *cm);

void handle_net_active_window_msg(WM *wm, XClientMessageEvent *cm);

void update_net_clients(WM *wm);

void update_net_workarea(WM *wm);

void reparent(WM *wm, Client *c);

void unparent(WM *wm, Client *c);

void handle_buttonpress(WM *wm, XButtonEvent *ev);

void handle_property_notify(WM *wm, XPropertyEvent *ev);

bool get_any_strut(WM *wm, Dock *dock);

Dock *win_in_docks(Window win);

void handle_net_close_window_msg(WM *wm, XClientMessageEvent *cm);

void set_net_active_window(WM *wm, Window win);

bool subwin_unmapped = false;

static Dock docks[4];
static int ndocks = 0;

void OnMapRequest(WM *wm, XMapRequestEvent* ev){
    XWindowAttributes wa;
    XGetWindowAttributes(wm->dpy, ev->window, &wa);
    if(wa.override_redirect)
        return;

    manage(wm, ev->window);
}

void OnConfigureRequest(WM *wm, XConfigureRequestEvent* ev){
    XWindowChanges changes = {0};

    changes.x = ev->x;
    changes.y = ev->y;
    changes.width = ev->width;
    changes.height = ev->height;
    changes.border_width = 0;
    changes.sibling = ev->above;
    changes.stack_mode = ev->detail;

    XConfigureWindow(wm->dpy, ev->window, ev->value_mask, &changes);
}

void OnDestroyNotify(WM *wm, XDestroyWindowEvent *ev){
    unmanage(wm, ev->window);
}

void OnButtonPress(WM *wm, XButtonEvent *ev){
    handle_buttonpress(wm, ev);
}

void OnPropertyNotify(WM *wm, XPropertyEvent *ev){
    handle_property_notify(wm, ev);
}

void handle_XEvent(WM *wm, XEvent *ev){
    switch(ev->type){
        case ButtonPress:
            OnButtonPress(wm, &ev->xbutton);    
            break;

        case ConfigureRequest:
            OnConfigureRequest(wm, &ev->xconfigurerequest);
            break;

        case MapRequest:
            OnMapRequest(wm, &ev->xmaprequest);
            break;

        case DestroyNotify:
            OnDestroyNotify(wm, &ev->xdestroywindow);
            break;

        case ClientMessage:
            handle_client_msg(wm, &ev->xclient);
            break;

        case PropertyNotify:
            OnPropertyNotify(wm, &ev->xproperty);
            break;
    }
}

void focus_direction(WM *wm, int dir) {
    Client *best = NULL;
    int bestdist = INT_MAX;


    XWindowAttributes fa;
    XGetWindowAttributes(wm->dpy, wm->workspaces[wm->current_ws].focused->parent, &fa);

    //get the center of the focused window
    int fx = fa.x + fa.width / 2;
    int fy = fa.y + fa.height / 2;

    for (Client *c = wm->clients; c; c = c->next) {
        if (c == wm->workspaces[wm->current_ws].focused) continue;
        if(!(c->wtags & wm->current_wtag)) continue;

        XWindowAttributes wa;
        XGetWindowAttributes(wm->dpy, c->parent, &wa);

        //get the center of the window that we are comparing
        int wx = wa.x + wa.width / 2;
        int wy = wa.y + wa.height / 2;

        //distance from the focused window center to the other window center
        int dx = wx - fx;
        int dy = wy - fy;

        bool valid = false;
        int dist = 0;

        //evaluate each calculated distance based on direction and penalize offset on the unfocused direction
        switch (dir) {
            case DIR_LEFT:
                valid = dx < 0;
                dist = -dx + abs(2 * dy);
                break;
            case DIR_RIGHT:
                valid = dx > 0;
                dist = dx + abs(2 * dy);
                break;
            case DIR_UP:
                valid = dy < 0;
                dist = -dy + abs(2 * dx);
                break;
            case DIR_DOWN:
                valid = dy > 0;
                dist = dy + abs(2 * dx);
                break;

            default:
                return;
        }

        //get the records
        if (valid && dist < bestdist) {
            best = c;
            bestdist = dist;
        }
    }

    if (best)
        focus(wm, best);
}

void monocle_focus(WM *wm, int dir){
    if(wm->nclients < 2) return;

    if(dir == DIR_RIGHT){
        if(wm->workspaces[wm->current_ws].focused->next){
            focus(wm, wm->workspaces[wm->current_ws].focused->next);
        }
        else
            focus(wm, wm->clients);
    }

    else if(dir == DIR_LEFT){
        if(wm->workspaces[wm->current_ws].focused->prev)
            focus(wm, wm->workspaces[wm->current_ws].focused->prev);

        else
            focus(wm, wm->tail);

    }

    tile(wm);
}

void unmap(WM *wm){
    if(wm->nclients < 1) return;

    if(subwin_unmapped == false){
        XSetInputFocus(wm->dpy, wm->root, RevertToNone, CurrentTime);
        set_net_active_window(wm, None);
        for(Client *c = wm->clients; c; c = c->next){
            XUnmapWindow(wm->dpy, c->win);
            XUnmapWindow(wm->dpy, c->parent);
        }
        subwin_unmapped = true;
    }
    else{
        tile(wm);
        focus(wm, wm->workspaces[wm->current_ws].focused);
        subwin_unmapped = false;
    }
}

//TODO remove this after new system stabilizes
void cmd_kill(WM *wm, const Arg *arg){
    if(!wm->workspaces[wm->current_ws].focused) return;

    kill_client(wm, wm->workspaces[wm->current_ws].focused);
}

void kill_client(WM *wm, Client *c){
    if(!c) return;
    if(!(c->wtags & wm->current_wtag)) return;

    if(c->protocols & PROTO_DELETE){
        XEvent ev = {0};
        ev.type = ClientMessage;
        ev.xclient.window = c->win;
        ev.xclient.message_type = wm->atoms.wm_protocols;
        ev.xclient.format = 32;
        // l type because format is 32 bits
        ev.xclient.data.l[0] = wm->atoms.wm_delete_window;
        ev.xclient.data.l[1] = CurrentTime;

        XSendEvent(wm->dpy, c->win, False, NoEventMask, &ev);
        return;
    }
    else{
        //If client is not compliant
        XKillClient(wm->dpy, c->win);
    }
}

void manage(WM *wm, Window win){
    Wintype type = classify_window(wm, win);

    if(type == WIN_DOCK){
        manage_dock(wm, win);
        update_net_workarea(wm);
        tile(wm);
        return;
    }

    Client *c = calloc(1, sizeof(Client));
    if(!c) return;

    c->win = win;

    c->next = NULL;

    if(wm->tail){
        wm->tail->next = c;
        c->prev = wm->tail;
    } 
    else{
    wm->clients = c;
    c->prev = NULL;
    }

    wm->tail = c;
    wm->nclients++;

    set_protocols(wm, c);
    c->wtags = Tag(wm->current_ws);

    if(type == WIN_DIALOG || type == WIN_SPLASH || type == WIN_MENU){
        c->floating = true;
    }

    if(c->floating){
        Window parent = get_transient(wm, win);
        if(parent) parent_center(wm, parent, c);
        else screen_center(wm, c);
    }

    update_net_clients(wm);

    if(!c->floating) 
        reparent(wm, c);

    XAddToSaveSet(wm->dpy, c->win);

    if(!c->floating){
        tile(wm);
    }

    focus(wm, c);
}

void unmanage(WM *wm, Window win){
    Wintype type = classify_window(wm, win);

    if(type == WIN_DOCK){
        unmanage_dock(wm, win);
        update_net_workarea(wm);
        tile(wm);
        return;
    }

    Client *c = win_in_clients(wm, win);
    if(!c) return;

    bool was_focused = (c == wm->workspaces[wm->current_ws].focused);


    if(c->prev)
        c->prev->next = c->next;
    else
        wm->clients = c->next;

    if(c->next)
        c->next->prev = c->prev;
    else
        wm->tail = c->prev;

    //handles focus by finding the next visible client
    if(was_focused){
        Client *next = c->next;
        while(next && !(next->wtags & wm->current_wtag))
            next = next->next;

        Client *prev = c->prev;
        while(!next && prev && !(prev->wtags & wm->current_wtag))
            prev = prev->prev;

        if(next)
            focus(wm, next);
        else if(prev)
            focus(wm, prev);
        else{
            wm->workspaces[wm->current_ws].focused = NULL;
            XSetInputFocus(wm->dpy, wm->root, RevertToNone, CurrentTime);
        }
    }

    wm->nclients--;

    update_net_clients(wm);

    unparent(wm, c);

    if(!wm->workspaces[wm->current_ws].focused){
        set_net_active_window(wm, None);
    }
    else{
        set_net_active_window(wm, wm->workspaces[wm->current_ws].focused->win);
    }

    tile(wm);
    free(c);
}

void focus(WM *wm, Client *c){
    if(!c) return;

    if(!(c->wtags & wm->current_wtag)) return;

    if(wm->workspaces[wm->current_ws].focused)
        XSetWindowBorder(wm->dpy, wm->workspaces[wm->current_ws].focused->parent, wm->config.inactive_border_color);

    wm->workspaces[wm->current_ws].focused = c;

    if(c->protocols & PROTO_TAKE_FOCUS){
        XEvent ev = {0};

        ev.xclient.type = ClientMessage;
        ev.xclient.window = c->win;
        ev.xclient.message_type = wm->atoms.wm_protocols;
        ev.xclient.format = 32;
        ev.xclient.data.l[0] = wm->atoms.wm_take_focus;
        ev.xclient.data.l[1] = CurrentTime;

        XSendEvent(wm->dpy, c->win, False, NoEventMask, &ev);
    }

    XSetInputFocus(wm->dpy, c->win, RevertToParent, CurrentTime);

    set_net_active_window(wm, c->win);

    XSetWindowBorder(wm->dpy, wm->workspaces[wm->current_ws].focused->parent, wm->config.active_border_color);

    XRaiseWindow(wm->dpy, c->win);
    if(c->parent) XRaiseWindow(wm->dpy, c->parent);

}

void set_master(WM *wm){
    if(!wm->clients) return;

    Workspace *ws = &wm->workspaces[wm->current_ws];
    Client *c = ws->focused;

    if(!c || c->floating || !(c->wtags & wm->current_wtag) || c == wm->clients) return;

    if(c->prev)
        c->prev->next = c->next;
    else
        wm->clients = c->next;

    if(c->next)
        c->next->prev = c->prev;
    else
        wm->tail = c->prev;

    c->next = wm->clients;
    c->prev = NULL;

    wm->clients->prev = c;

    wm->clients = c;

    tile(wm);
}

Client* win_in_clients(WM *wm, Window win){
    for(Client *c = wm->clients; c; c = c->next){
        if(c->win == win){
            return c;
        }
    }
    return NULL;
}

int has_wintype(int nitems, Atom *atoms, Atom type){
    int res = 0;

    for(int i = 0; i < nitems; i++){
        if(atoms[i] == type){
            res = 1;
            break;
        }
    }
    return res;
}

bool get_strut_partial(WM *wm, Dock *dock){
    Atom actual_type;
    int actual_format;
    unsigned long nitems, bytes_after;
    unsigned char *data = NULL;

    if(XGetWindowProperty(
                wm->dpy,
                dock->win,
                wm->atoms.net_wm_strut_partial,
                0,
                12,
                False,
                XA_CARDINAL,
                &actual_type,
                &actual_format,
                &nitems,
                &bytes_after,
                &data)!= Success)
        return false;

    if(!data || nitems < 12){
        if(data)
            XFree(data);
        return false;
    }

    long *strut = (long *)data;

    dock->left = strut[0];
    dock->right = strut[1];
    dock->top = strut[2];
    dock->bottom = strut[3];
    dock->left_start_y = strut[4];
    dock->left_end_y = strut[5];
    dock->right_start_y = strut[6];
    dock->right_end_y = strut[7];
    dock->top_start_x = strut[8];
    dock->top_end_x = strut[9];
    dock->bottom_start_x = strut[10];
    dock->bottom_end_x =strut[11];

    XFree(data);
    return true;
}

bool get_strut(WM *wm, Dock *dock){
    Atom actual_type;
    int actual_format;
    unsigned long nitems, bytes_after;
    unsigned char *data = NULL;

    if(XGetWindowProperty(
                wm->dpy,
                dock->win,
                wm->atoms.net_wm_strut,
                0,
                4,
                False,
                XA_CARDINAL,
                &actual_type,
                &actual_format,
                &nitems,
                &bytes_after,
                &data)!= Success)
        return false;

    if(!data || nitems < 4){
        if(data)
            XFree(data);
        return false;
    }

    long *strut = (long *)data;

    dock->left = strut[0];
    dock->right = strut[1];
    dock->top = strut[2];
    dock->bottom = strut[3];

    XFree(data);
    return true;
}

void recalc_usable_area(WM *wm){
    int left = 0, right = 0, bottom = 0, top = 0;

    for(int i = 0; i < ndocks; i++){
        if(docks[i].left > left) left = docks[i].left;
        if(docks[i].right > right) right = docks[i].right;
        if(docks[i].bottom > bottom)  bottom = docks[i].bottom;
        if(docks[i].top > top) top = docks[i].top;
    }

    wm->usable_height = wm->sh - top - bottom;
    wm->usable_width = wm->sw - right - left;

    wm->usable_x = left;
    wm->usable_y = top;
}

void unmanage_dock(WM *wm, Window win){
    (void)wm;
    int idx = -1;

    for(int i = 0; i < ndocks; i++){
        if(docks[i].win == win){
            idx = i;
            break;
        }
    }
    if(idx == -1) return;

    for(int i = idx; i < ndocks-1; i++){
        docks[i] = docks[i + 1];
    }

    ndocks--;
    recalc_usable_area(wm);
}

Wintype classify_window(WM *wm, Window win){
    Atom actual_type;
    int actual_format;
    unsigned long nitems, bytes_after;
    unsigned char *data = NULL;

    Wintype type = WIN_NORMAL;

    if(XGetWindowProperty(
                wm->dpy,
                win,
                wm->atoms.net_wm_win_type,
                0,
                32,
                False,
                XA_ATOM,
                &actual_type,
                &actual_format,
                &nitems,
                &bytes_after,
                &data) != Success)
        return WIN_NORMAL;

    if(actual_type != XA_ATOM || actual_format != 32 || data == NULL)
        goto cleanup;

    //Recast to atom ptr because data is unsigned char initially
    Atom *atoms = (Atom *)data;

    if(has_wintype(nitems, atoms, wm->atoms.net_wm_win_type_dialog)){
        type = WIN_DIALOG;
        goto cleanup;
    }

    if(has_wintype(nitems, atoms, wm->atoms.net_wm_win_type_menu)){
        type = WIN_MENU;
        goto cleanup;
    }

    if(has_wintype(nitems, atoms, wm->atoms.net_wm_win_type_dock)){
        type = WIN_DOCK;
        goto cleanup;
    }

    if(has_wintype(nitems, atoms, wm->atoms.net_wm_win_type_splash)){
        type = WIN_SPLASH;
        goto cleanup;
    }

    Window parent;
    if(XGetTransientForHint(wm->dpy, win, &parent)){
        type = WIN_DIALOG;
    }

cleanup:
    if(data)
        XFree(data);

    return type;
}

void manage_dock(WM *wm, Window win){
    if(ndocks == MAX_DOCKS)
        return;

    Dock *dock = &docks[ndocks];
    *dock = (Dock){0};
    dock->win = win;

    if(!get_any_strut(wm, dock)){
        printf("Failed to acquire strut parameters\n");
        return;
    }

    ndocks++;

    recalc_usable_area(wm);

    XMapWindow(wm->dpy, win);

    return;
}

Window get_transient(WM *wm, Window win){
    Window transient = None;

    if(XGetTransientForHint(wm->dpy, win, &transient))
        return transient;

    return None;
}

void set_protocols(WM *wm, Client *c){

    c->protocols = 0;

    Atom *protocols;
    int nproto;

    if(XGetWMProtocols(wm->dpy, c->win, &protocols, &nproto)){
        for(int i = 0; i < nproto; i++){

            if(protocols[i] == wm->atoms.wm_delete_window)
                c->protocols |= PROTO_DELETE;

            if(protocols[i] == wm->atoms.wm_take_focus)
                c->protocols |= PROTO_TAKE_FOCUS;
        }
    }

    XFree(protocols);
    return;
}

void init_atoms(WM *wm){
    wm->atoms.net_wm_win_type = XInternAtom(wm->dpy, "_NET_WM_WINDOW_TYPE", False);
    wm->atoms.net_wm_win_type_dock = XInternAtom(wm->dpy, "_NET_WM_WINDOW_TYPE_DOCK", False);
    wm->atoms.net_wm_strut_partial = XInternAtom(wm->dpy, "_NET_WM_STRUT_PARTIAL", False);
    wm->atoms.net_wm_strut = XInternAtom(wm->dpy, "_NET_WM_STRUT", False);

    wm->atoms.net_wm_win_type_dialog = XInternAtom(wm->dpy, "_NET_WM_WINDOW_TYPE_DIALOG", False);
    wm->atoms.net_wm_win_type_menu = XInternAtom(wm->dpy, "_NET_WM_WINDOW_TYPE_MENU", False);
    wm->atoms.net_wm_win_type_splash = XInternAtom(wm->dpy, "_NET_WM_WINDOW_TYPE_SPLASH", False);
    wm->atoms.net_wm_win_type_normal = XInternAtom(wm->dpy, "_NET_WM_WINDOW_TYPE_NORMAL", False);

    wm->atoms.net_active_window = XInternAtom(wm->dpy, "_NET_ACTIVE_WINDOW", False);
    wm->atoms.net_client_list = XInternAtom(wm->dpy, "_NET_CLIENT_LIST", False);
    wm->atoms.net_num_of_desktops = XInternAtom(wm->dpy, "_NET_NUMBER_OF_DESKTOPS", False);
    wm->atoms.net_current_desktop = XInternAtom(wm->dpy, "_NET_CURRENT_DESKTOP", False);
    wm->atoms.net_workarea = XInternAtom(wm->dpy, "_NET_WORKAREA", False);
    wm->atoms.net_supp_wm_check = XInternAtom(wm->dpy, "_NET_SUPPORTING_WM_CHECK", False);
    wm->atoms.net_close_window = XInternAtom(wm->dpy, "_NET_CLOSE_WINDOW", False);

    wm->atoms.wm_protocols = XInternAtom(wm->dpy, "WM_PROTOCOLS", False);
    wm->atoms.wm_delete_window = XInternAtom(wm->dpy, "WM_DELETE_WINDOW", False);
    wm->atoms.wm_take_focus = XInternAtom(wm->dpy, "WM_TAKE_FOCUS", False);
}

void init_layouts(WM *wm){
    //order must match LayoutID order
    wm->layouts[LAYOUT_MASTER] = master_layout();
    wm->layouts[LAYOUT_MONOCLE] = monocle_layout();
}

void cycle_layout(WM *wm){
    Workspace *ws = &wm->workspaces[wm->current_ws];

    ws->layout_id = (ws->layout_id + 1) % 2; // 2 is the num of layouts
    ws->layout = &wm->layouts[ws->layout_id];

    tile(wm);
}


void handle_client_msg(WM *wm, XClientMessageEvent *cm){
    if(cm->message_type == wm->atoms.net_active_window){
        handle_net_active_window_msg(wm, cm);
    }
    else if(cm->message_type == wm->atoms.net_close_window){
        handle_net_close_window_msg(wm, cm);
    }
}

void handle_net_active_window_msg(WM *wm, XClientMessageEvent *cm){
    Client *c = win_in_clients(wm, cm->window);

    if(c){
        focus(wm, c);

        XRaiseWindow(wm->dpy, c->win);
    }
}

void handle_net_close_window_msg(WM *wm, XClientMessageEvent *cm){
    Client *c = win_in_clients(wm, cm->window);

    if(c){
        kill_client(wm, c);
    }
}

void update_net_clients(WM *wm){
    Window net_clients[wm->nclients];

    int i = 0;
    for(Client *c = wm->clients; c; c = c->next){
        net_clients[i] = c->win;
        i++;
    }

    XChangeProperty(
        wm->dpy,
        wm->root,
        wm->atoms.net_client_list,
        XA_WINDOW,
        32,
        PropModeReplace,
        (unsigned char *)net_clients,
        wm->nclients
    );
}

void initset_net_supported(WM *wm){
    Atom net_supported = XInternAtom(wm->dpy, "_NET_SUPPORTED", False);

    Atom supported[] = {
        wm->atoms.net_wm_strut_partial,
        wm->atoms.net_wm_strut,
        wm->atoms.net_wm_win_type,
        wm->atoms.net_wm_win_type_dock,
        wm->atoms.net_wm_win_type_dialog,
        wm->atoms.net_wm_win_type_menu,
        wm->atoms.net_wm_win_type_splash,
        wm->atoms.net_wm_win_type_normal,
        wm->atoms.net_active_window,
        wm->atoms.net_client_list,
        wm->atoms.net_num_of_desktops,
        wm->atoms.net_current_desktop,
        wm->atoms.net_workarea,
        wm->atoms.net_supp_wm_check,
        wm->atoms.net_close_window
    };

    int nsupp = sizeof(supported) / sizeof(supported[0]);

    XChangeProperty(wm->dpy,
            wm->root,
            net_supported,
            XA_ATOM,
            32,
            PropModeReplace,
            (unsigned char *)supported,
            nsupp
    );
}

void update_net_num_of_desktops(WM *wm){
    long n = MAX_WS;

    XChangeProperty(
        wm->dpy,
        wm->root,
        wm->atoms.net_num_of_desktops,
        XA_CARDINAL,
        32,
        PropModeReplace,
        (unsigned char *)&n,
        1
    );
}

void update_net_current_desktop(WM *wm){
    long n = wm->current_ws;

    XChangeProperty(
        wm->dpy,
        wm->root,
        wm->atoms.net_current_desktop,
        XA_CARDINAL,
        32,
        PropModeReplace,
        (unsigned char *)&n,
        1
    );
}

void update_net_workarea(WM *wm){
    long workarea[] = {wm->usable_x, wm->usable_y, wm->usable_width, wm->usable_height};

    XChangeProperty(
        wm->dpy,
        wm->root,
        wm->atoms.net_workarea,
        XA_CARDINAL,
        32,
        PropModeReplace,
        (unsigned char *)&workarea,
        4
    );
}

void set_net_supp_wm_check(WM *wm){
    Window check_win = XCreateSimpleWindow(wm->dpy, wm->root, 0, 0, 1, 1, 0, 0, 0);

    Atom net_wm_name = XInternAtom(wm->dpy, "_NET_WM_NAME", False);
    Atom utf8 = XInternAtom(wm->dpy, "UTF8_STRING", False);
     const char *name = "moss";

    XChangeProperty(
        wm->dpy,
        wm->root,
        wm->atoms.net_supp_wm_check,
        XA_WINDOW,
        32,
        PropModeReplace,
        (unsigned char *)&check_win,
        1
    );

    XChangeProperty(
        wm->dpy,
        check_win,
        wm->atoms.net_supp_wm_check,
        XA_WINDOW,
        32,
        PropModeReplace,
        (unsigned char *)&check_win,
        1
    );

    XChangeProperty(
        wm->dpy,
        check_win,
        net_wm_name,
        utf8,
        8,
        PropModeReplace,
        (unsigned char *)name,
        strlen(name)
    );
}

void reparent(WM *wm, Client *c){
    if(c->floating) return;

    XWindowAttributes ca;
    XGetWindowAttributes(wm->dpy, c->win, &ca);

    XSetWindowAttributes attrs;
    attrs.override_redirect = True;

    Window parent = XCreateWindow(
        wm->dpy,
        wm->root,
        0, 0,
        ca.width, 
        ca.height,
        wm->config.border_width,
        CopyFromParent,
        InputOutput,
        CopyFromParent,
        CWOverrideRedirect,
        &attrs);

    XReparentWindow(wm->dpy, c->win, parent, 0, 0);
    c->parent = parent;

    XGrabButton(
            wm->dpy,
            Button1,
            0,
            c->parent,
            False,
            ButtonPressMask,
            GrabModeSync,
            GrabModeAsync,
            None,
            None);

    XSelectInput(wm->dpy, c->win, StructureNotifyMask);
}

void unparent(WM *wm, Client *c){
    if(!c->parent || c->floating) return;

    XDestroyWindow(wm->dpy, c->parent);
}

void handle_buttonpress(WM *wm, XButtonEvent *ev){
    //subwindow bc grabs are done on the frame, not the client
    Window win = ev->subwindow;
    Client *c = win_in_clients(wm, win);

    if(!c){
        XAllowEvents(wm->dpy, ReplayPointer, CurrentTime);
        return;
    }

    focus(wm, c);

    XAllowEvents(wm->dpy, ReplayPointer, CurrentTime);

    XSync(wm->dpy, False);
}

void send_conf_req(WM *wm, Client *c, int width, int height, int x, int y){
    XConfigureEvent ev;

    ev.type = ConfigureNotify;
    ev.display = wm->dpy;
    ev.event = c->win;
    ev.window = c->win;

    ev.width = width;
    ev.height = height;
    ev.x = x;
    ev.y = y;

    ev.border_width = 0;
    ev.above = None;
    ev.override_redirect = False;

    XSendEvent(wm->dpy, c->win, False, StructureNotifyMask, (XEvent *)&ev);
}

bool get_any_strut(WM *wm, Dock *dock){
    if(!dock) return false;

    if(!get_strut_partial(wm, dock)){
        if(!get_strut(wm, dock))
            return false;
    }

    return true;
}

void handle_property_notify(WM *wm, XPropertyEvent *ev){
    if(ev->atom == wm->atoms.net_wm_strut_partial || ev->atom == wm->atoms.net_wm_strut){
        Dock *dock = win_in_docks(ev->window);
        if(!dock)
            return;

        get_any_strut(wm, dock);
        recalc_usable_area(wm);
    }
}

Dock *win_in_docks(Window win){
    for(int i = 0; i < ndocks; i++){
        if(win == docks[i].win)
            return &docks[i];
    }

    return NULL;
}

void init_workspaces(WM *wm){
    for(int i = 0; i < MAX_WS; i++){
        wm->workspaces[i].layout = &wm->layouts[0];
        wm->workspaces[i].layout_id = LAYOUT_MASTER;
        wm->workspaces[i].focused = NULL;
        wm->workspaces[i].mfactor = 0.5;
        wm->workspaces[i].master_pos = MASTER_LEFT;
    }

    wm->current_ws = 0;
    wm->current_wtag = Tag(wm->current_ws);
}

void switch_workspace(WM *wm, int dir){
    if(dir == DIR_RIGHT){
        wm->current_ws = (wm->current_ws + 1) % MAX_WS;
    }
    else if(dir == DIR_LEFT){
        wm->current_ws = (wm->current_ws - 1 + MAX_WS) % MAX_WS;
    }

    wm->current_wtag = Tag(wm->current_ws);

    update_net_current_desktop(wm);

    tile(wm);

    if(wm->workspaces[wm->current_ws].focused){
        focus(wm, wm->workspaces[wm->current_ws].focused);
    }
    else{
        XSetInputFocus(wm->dpy, wm->root, RevertToNone, CurrentTime);
        set_net_active_window(wm, None);
    }
}

void move_cli_ws(WM *wm, int index){
    if(MAX_WS - 1 < index || index < 0) return;

    Client *c = wm->workspaces[wm->current_ws].focused;
    if(!c) return;

    Client *next = c->next;
    while(next && !(next->wtags & wm->current_wtag))
        next = next->next;

    Client *prev = c->prev;
    while(!next && prev && !(prev->wtags & wm->current_wtag))
        prev = prev->prev;

    if(next)
        wm->workspaces[wm->current_ws].focused = next;
    else if(prev)
        wm->workspaces[wm->current_ws].focused = prev;
    else{
        wm->workspaces[wm->current_ws].focused = NULL;
    }

    c->wtags = Tag(index);
    wm->current_ws = index;
    wm->current_wtag = Tag(index);

    update_net_current_desktop(wm);

    tile(wm);

    focus(wm, c);
}

void spawn(WM *wm, const Arg *arg){
    (void)wm;
    if(fork() == 0){
        setsid(); // Create a new session for the child
        execvp(arg->cparr[0], (char *const *)arg->cparr);

        //If child fails
        perror("execvp failed");
        _exit(1); // A safe way to terminate the forked child
    }
}

void init_config(WM *wm){
    wm->config.active_border_color = 0x4488FF;
    wm->config.inactive_border_color = 0x71797E;
    wm->config.border_width = 4;
    wm->config.conf_addr = "/home/ilya/.config/moss/moss.conf";
}

void set_net_active_window(WM *wm, Window win){
    XChangeProperty(wm->dpy, wm->root, wm->atoms.net_active_window, XA_WINDOW, 32, PropModeReplace, (unsigned char *)&win, 1);
}
