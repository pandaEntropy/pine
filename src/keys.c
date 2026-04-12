#include "keys.h"
#include "commands.h"
#include "wm.h"

#include <X11/keysym.h>
#include <X11/Xlib.h>

typedef struct{
    KeySym keysym;
    unsigned int mod;
    const char *cmd;
} Key;

Key keys[] = {
    {XK_h, Mod1Mask, "focus_left"},
    {XK_l, Mod1Mask, "focus_right"},
    {XK_k, Mod1Mask, "focus_up"},
    {XK_j, Mod1Mask, "focus_down"},
    {XK_Return, Mod1Mask, "spawn"},
    {XK_r, Mod1Mask, "rotate"},
    {XK_d, Mod1Mask, "unmap"},
    {XK_x, Mod1Mask, "kill"},
    {XK_l, Mod1Mask | ControlMask, "resize_right"},
    {XK_h, Mod1Mask | ControlMask, "resize_left"},
    {XK_k, Mod1Mask | ControlMask, "resize_up"},
    {XK_j, Mod1Mask | ControlMask, "resize_down"},
    {XK_Return, Mod1Mask | ControlMask, "set_master"},
    {XK_m, Mod1Mask, "switch_layout"},
    {XK_Right, Mod1Mask | ControlMask, "switch_workspace_right"},
    {XK_Left, Mod1Mask | ControlMask, "switch_workspace_left"},
    {XK_1, Mod1Mask | ControlMask, "switch_cli_ws_1"},
    {XK_2, Mod1Mask | ControlMask, "switch_cli_ws_2"},
    {XK_3, Mod1Mask | ControlMask, "switch_cli_ws_3"},
    {XK_4, Mod1Mask | ControlMask, "switch_cli_ws_4"},
    {XK_5, Mod1Mask | ControlMask, "switch_cli_ws_5"},
    {XK_6, Mod1Mask | ControlMask, "switch_cli_ws_6"},
    {XK_7, Mod1Mask | ControlMask, "switch_cli_ws_7"},
    {XK_8, Mod1Mask | ControlMask, "switch_cli_ws_8"},
    {XK_9, Mod1Mask | ControlMask, "switch_cli_ws_9"},
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
            dispatch_command(wm, keys[i].cmd);
            return;
        }
    }
}
