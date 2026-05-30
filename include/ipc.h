#ifndef IPC_H
#define IPC_H

#include "forward.h"

#define SOCK_PATH "/tmp/wm.sock"
#define SOCK_BUFSIZE 128

extern int wmfd;

void ipc_init();

void ipc_handle(WM *wm);

#endif 
