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
    {XK_Right, Mod1Mask | ShiftMask, "switch_workspace_right"},
    {XK_Left, Mod1Mask | ShiftMask, "switch_workspace_left"}
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
