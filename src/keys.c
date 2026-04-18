#include "keys.h"
#include "wm.h"
#include "layout.h"

#include <X11/keysym.h>
#include <X11/Xlib.h>
#include <stdio.h>
#include <string.h>

typedef struct{
    char *name;
    KeySym keysym;
    unsigned int mod;
    void (*func)(WM *wm, const Arg *arg);
    Arg arg;
} Key;

typedef struct{
    char *name;
    unsigned int mod;
}ModMap;

char *termcmd[] = {"xterm", NULL};

//TODO add an option to change the workspace opertaion key (ctrl + alt by default) -> for switch_cli_ws especially

Key keys[] = {
    {"focus_left", XK_h, Mod1Mask, cmd_focus, {.i = DIR_LEFT}},
    {"focus_right", XK_l, Mod1Mask, cmd_focus, {.i = DIR_RIGHT}},
    {"focus_up", XK_k, Mod1Mask, cmd_focus, {.i = DIR_UP}},
    {"focus_down", XK_j, Mod1Mask, cmd_focus, {.i = DIR_DOWN}},
    {"spawn", XK_Return, Mod1Mask, spawn, {.cparr = termcmd}}, //TODO revise spawning
    {"rotate", XK_r, Mod1Mask, rotate, {0}},
    {"unmap", XK_d, Mod1Mask, unmap, {0}},
    {"kill_window", XK_x, Mod1Mask, cmd_kill, {0}},
    {"resize_right", XK_l, Mod1Mask | ControlMask, resize, {.i = DIR_RIGHT}},
    {"resize_left", XK_h, Mod1Mask | ControlMask, resize, {.i = DIR_LEFT}},
    {"resize_up", XK_k, Mod1Mask | ControlMask, resize, {.i = DIR_UP}},
    {"resize_down", XK_j, Mod1Mask | ControlMask, resize, {.i = DIR_DOWN}},
    {"set_master", XK_Return, Mod1Mask | ControlMask, set_master, {0}},
    {"switch_layout", XK_m, Mod1Mask, switch_layout, {0}},
    {"switch_workspace_right", XK_Right, Mod1Mask | ControlMask, switch_workspace, {.i = DIR_RIGHT}},
    {"switch_workspace_right", XK_Left, Mod1Mask | ControlMask, switch_workspace, {.i = DIR_LEFT}},
    {"switch_cli_ws", XK_1, Mod1Mask | ControlMask, switch_cli_ws, {.i = 0}},
    {"switch_cli_ws", XK_2, Mod1Mask | ControlMask, switch_cli_ws, {.i = 1}},
    {"switch_cli_ws", XK_3, Mod1Mask | ControlMask, switch_cli_ws, {.i = 2}},
    {"switch_cli_ws", XK_4, Mod1Mask | ControlMask, switch_cli_ws, {.i = 3}},
    {"switch_cli_ws", XK_5, Mod1Mask | ControlMask, switch_cli_ws, {.i = 4}},
    {"switch_cli_ws", XK_6, Mod1Mask | ControlMask, switch_cli_ws, {.i = 5}},
    {"switch_cli_ws", XK_7, Mod1Mask | ControlMask, switch_cli_ws, {.i = 6}},
    {"switch_cli_ws", XK_8, Mod1Mask | ControlMask, switch_cli_ws, {.i = 7}},
    {"switch_cli_ws", XK_9, Mod1Mask | ControlMask, switch_cli_ws, {.i = 8}}
};

ModMap mod_map[] = {
    {"Alt", Mod1Mask},
    {"Control", ControlMask},
    {"Shift", ShiftMask},
    {"Meta", Mod4Mask}
};

void grab_key(Display *dpy, KeySym keysym, unsigned int mod){
    KeyCode code = XKeysymToKeycode(dpy, keysym);
    XGrabKey(dpy, code, mod, DefaultRootWindow(dpy), False, GrabModeAsync, GrabModeAsync);
}

void key_setup(Display *dpy){
    for(size_t i = 0; i < (sizeof(keys) / sizeof(Key)); i++){
        grab_key(dpy, keys[i].keysym, keys[i].mod);  
    }
}

void handle_keypress(WM *wm, XKeyEvent *ev){
    for(size_t i = 0; i < (sizeof(keys) / sizeof(Key)); i++){
        if(ev->keycode == XKeysymToKeycode(wm->dpy, keys[i].keysym) && ev->state == keys[i].mod){
            keys[i].func(wm, &keys[i].arg);
            return;
        }
    }
}

void bind_key(WM *wm, char *name, KeySym sym, unsigned int modmask){
    printf("%s\n", name);
    for(int i = 0; i < (sizeof(keys) / sizeof(Key)); i++){
        if(strcmp(name, keys[i].name) == 0){
            keys[i].keysym = sym;
            keys[i].mod = modmask;
            grab_key(wm->dpy, sym, modmask);
            return;
        }
    }
    fprintf(stderr, "bind_key: key not found\n");
}

unsigned int parse_mod(char *mod_token){
    for(int i = 0; i < sizeof(mod_map) / sizeof(ModMap); i++){
        if(strcmp(mod_token, mod_map[i].name) == 0){
            return mod_map[i].mod;
        }
    }
    fprintf(stderr, "parse_mod: modifier not found\n");
    return 0;
}
