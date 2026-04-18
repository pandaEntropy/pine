#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/keysym.h>
#include <X11/cursorfont.h>
#include <errno.h>
#include <stdio.h>

#include "wm.h"
#include "keys.h"
#include "ipc.h"
#include "config.h"

static int startup_err = 0;

int startup_handler(Display *dpy, XErrorEvent *ev){
    (void)dpy;
    if(ev->error_code == BadAccess)
        startup_err = 1;

    return 0;
}

int general_handler(Display *dpy, XErrorEvent *ev){
    if(ev->error_code == BadWindow)
        return 0;

    char buf[256];
    XGetErrorText(dpy, ev->error_code, buf, sizeof(buf));

    fprintf(stderr, "X Error: %s (request = %d)\n", buf, ev->request_code);

    return 0;
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
        fprintf(stderr, "Another WM is currently running\n");
        return 1;
    }

    XSetErrorHandler(general_handler);

    //disables focus on hover
    XSetInputFocus(wm.dpy, None, RevertToParent, CurrentTime);

    key_setup(wm.dpy); //grabs all the necessary keys

    XGrabButton(wm.dpy, 1, Mod1Mask, wm.root, True,
            ButtonPressMask|ButtonReleaseMask|PointerMotionMask, GrabModeAsync, GrabModeAsync, None, None);
    XGrabButton(wm.dpy, 3, Mod1Mask, wm.root, True,
            ButtonPressMask|ButtonReleaseMask|PointerMotionMask, GrabModeAsync, GrabModeAsync, None, None);

    //Create and define the cursor
    Cursor cursor = XCreateFontCursor(wm.dpy, XC_left_ptr);
    XDefineCursor(wm.dpy, DefaultRootWindow(wm.dpy), cursor);

    //Initialize atoms in the wm struct
    init_atoms(&wm);

    set_net_supp_wm_check(&wm);

    //Set the net supported property on root
    initset_net_supported(&wm);

    wm.usable_height = wm.sh;
    wm.usable_width = wm.sw;

    init_layouts(&wm);

    update_net_num_of_desktops(&wm);
    update_net_current_desktop(&wm);
    init_workspaces(&wm);

    load_config(&wm, "/home/ilya/.config/moss/moss.conf");

    XSync(wm.dpy, False);

    ipc_init();

    int xfd = ConnectionNumber(wm.dpy);

    for(;;){
        fd_set fds;
        FD_ZERO(&fds); //clear the bitmask
        FD_SET(xfd, &fds); 
        FD_SET(wmfd, &fds);

        int maxfd = (xfd > wmfd ? xfd : wmfd) + 1;

        if(select(maxfd, &fds, NULL, NULL, NULL) < 0){
            if(errno == EINTR) continue;
            break;
        }

        if(FD_ISSET(wmfd, &fds)){
            //ipc_handle(&wm); TODO fix
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
    XCloseDisplay(wm.dpy);
}

