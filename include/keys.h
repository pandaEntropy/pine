#ifndef KEYS_H
#define KEYS_H

#include <X11/Xlib.h>

#include "forward.h"

void grab_key(Display *dpy, KeySym keysym, unsigned int mod);

void key_setup(Display *dpy);

void handle_keypress(WM *wm, XKeyEvent *ev);

void bind_key(WM *wm, char *name, KeySym sym, unsigned int modmask);

unsigned int parse_mod(char *mod_token);

#endif
