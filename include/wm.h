#ifndef MYWM_H
#define MYWM_H

#include <X11/Xlib.h>
#include <stdbool.h>
#include <stdint.h>

#include "forward.h"

typedef struct Client{
    Window win;
    struct Client *next;
    struct Client *prev;
    bool floating;
    unsigned int protocols;
    Window parent;

    uint32_t wtags; //workspace tags
}Client;

typedef struct Dock{
    Window win;
    int left;
    int right;
    int top;
    int bottom;
    int left_start_y;
    int left_end_y;
    int right_start_y;
    int right_end_y;
    int top_start_x;
    int top_end_x;
    int bottom_start_x;
    int bottom_end_x;
}Dock;

typedef struct Arg{
    int i;
    const char **cparr;
}Arg;

typedef enum Direction{
    DIR_LEFT,
    DIR_DOWN,
    DIR_RIGHT,
    DIR_UP
}Direction;

typedef enum{
    LAYOUT_MASTER,
    LAYOUT_MONOCLE
}LayoutID;

typedef struct Layout{
    LayoutID id;
    void (*tile)(WM *, LayoutTarget *targets);
    void (*rotate)(WM *);
    void (*focus)(WM *, int);
}Layout;

typedef struct Atoms{
    Atom net_wm_win_type;
    Atom net_wm_win_type_dock;
    Atom net_wm_strut_partial;
    Atom net_wm_strut;

    Atom net_wm_win_type_dialog;
    Atom net_wm_win_type_menu;
    Atom net_wm_win_type_splash;
    Atom net_wm_win_type_normal;

    Atom net_active_window;
    Atom net_client_list;
    Atom net_num_of_desktops;
    Atom net_current_desktop;
    Atom net_workarea;
    Atom net_supp_wm_check;
    Atom net_close_window;

    Atom wm_protocols;
    Atom wm_take_focus;
    Atom wm_delete_window;
 
}Atoms;

typedef struct Workspace{
    LayoutID layout_id;
    Layout *layout;
    int master_pos;

    float mfactor;
    Client *focused;
}Workspace;

typedef struct WM{
    Display *dpy;
    Window root;
    int sw;
    int sh;

    Client *clients;
    Client *tail;
    int nclients;
    int usable_height;
    int usable_width;

    Workspace workspaces[9];
    int current_ws;
    uint32_t current_wtag;

    //first usable point from the origin
    int usable_x; 
    int usable_y;

    Atoms atoms;

    Layout layouts[8];
}WM;

void handle_XEvent(WM *wm, XEvent *ev);

void focus(WM *wm, Client *c);

void focus_direction(WM *wm, int dir);

void unmap(WM *wm, const Arg *arg);

void cmd_kill(WM *wm, const Arg *arg);

void set_master(WM *wm, const Arg *arg);

Client *win_in_clients(WM *wm, Window win);

int has_wintype(int nitems, Atom *atoms, Atom type);

Client* get_client(WM *wm, Window win); 

Window get_transient(WM *wm, Window win);

void set_protocols(WM *wm, Client *c);

void init_atoms(WM *wm);

Client *last_client(WM *wm);

void monocle_focus(WM *wm, int dir);

void cmd_focus(WM *wm, const Arg *arg);

void init_layouts(WM *wm);

void switch_layout(WM *wm, const Arg *arg);

void initset_net_supported(WM *wm);

void update_net_num_of_desktops(WM *wm);

void update_net_current_desktop(WM *wm);

void set_net_supp_wm_check(WM *wm);

void send_conf_req(WM *wm, Client *c, int width, int height, int x, int y);

void init_workspaces(WM *wm);

void switch_workspace(WM *wm, const Arg *arg);

#endif
