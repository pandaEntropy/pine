#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/keysym.h>
#include <X11/cursorfont.h>
#include <errno.h>
#include <stdio.h>
#include <signal.h>

#include "pine.h"
#include "ipc.h"
#include "config.h"

static int startup_err = 0;
static int running = 1;

int startup_handler(Display *dpy, XErrorEvent *ev){
    (void)dpy;
    if(ev->error_code == BadAccess)
        startup_err = 1;

    return 0;
}

int general_handler(Display *dpy, XErrorEvent *ev){
    if(ev->error_code == BadWindow ||
        ev->error_code == BadDrawable ||
        ev->error_code == BadMatch)
        return 0;

    char buf[256];
    XGetErrorText(dpy, ev->error_code, buf, sizeof(buf));

    fprintf(stderr, "XError: %s\n request=%d minor=%d resource=0x%lx\n", buf, ev->request_code, ev->minor_code, 
            ev->resourceid);

    return 0;
}

void handle_sigterm(int sig){
    running = 0;
}

int main(void)
{
    WM wm = {0};

    if(!(wm.dpy = XOpenDisplay(0x0))) return 1;

    int screen = DefaultScreen(wm.dpy);
    wm.sw = DisplayWidth(wm.dpy, screen);
    wm.sh = DisplayHeight(wm.dpy, screen);
    wm.root = DefaultRootWindow(wm.dpy);

    XSetErrorHandler(startup_handler);

    XSelectInput(wm.dpy, wm.root, SubstructureRedirectMask | 
            SubstructureNotifyMask |
            EnterWindowMask |
            LeaveWindowMask |
            FocusChangeMask |
            PropertyChangeMask);

    XSync(wm.dpy, False);

    if(startup_err){
        level_log(&wm, ERROR, "another client has selected substructure redirect, closing...");
        return 1;
    }

    XSetErrorHandler(general_handler);

    XSetInputFocus(wm.dpy, wm.root, RevertToParent, CurrentTime);

    XGrabButton(wm.dpy, 1, None, wm.root, True,
            ButtonPressMask|ButtonReleaseMask|PointerMotionMask, GrabModeSync, GrabModeAsync, None, None);
    XGrabButton(wm.dpy, 3, None, wm.root, True,
            ButtonPressMask|ButtonReleaseMask|PointerMotionMask, GrabModeSync, GrabModeAsync, None, None);

    //Create and define the cursor
    Cursor cursor = XCreateFontCursor(wm.dpy, XC_left_ptr);
    XDefineCursor(wm.dpy, DefaultRootWindow(wm.dpy), cursor);

    //Initialize atoms in the wm struct
    init_atoms(&wm);

    init_layouts(&wm);

    init_config(&wm);
    load_config(&wm);

    init_ewmh(&wm);

    wm.usable_height = wm.sh - (2 * wm.config.gap_size);
    wm.usable_width = wm.sw - (2 * wm.config.gap_size);
    wm.usable_x += wm.config.gap_size;
    wm.usable_y += wm.config.gap_size;

    XSync(wm.dpy, False);

    ipc_init();

    struct sigaction sa;
    sa.sa_handler = &handle_sigterm;
    sigaction(SIGTERM, &sa, NULL);

    int xfd = ConnectionNumber(wm.dpy);
    int range = (xfd > wmfd ? xfd : wmfd) + 1;
    while(running){
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(xfd, &fds); 
        FD_SET(wmfd, &fds);

        if(select(range, &fds, NULL, NULL, NULL) < 0){
            if(errno == EINTR) continue;
            break;
        }

        if(FD_ISSET(wmfd, &fds)){
            ipc_handle(&wm);
            XFlush(wm.dpy);
        }

        if(FD_ISSET(xfd, &fds)){
            while(XPending(wm.dpy)){
               XEvent ev;
               XNextEvent(wm.dpy, &ev);
               handle_XEvent(&wm, &ev);
            }
        }
    }

    cleanup(&wm);
}
