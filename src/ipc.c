#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include "ipc.h" 
#include "config.h"

#define IPC_PATH "/tmp/wm.sock"
#define IPC_BUFSIZE 128

int wmfd = -1;

void ipc_init(){
    struct sockaddr_un addr = {0};

    wmfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if(wmfd < 0){
        perror("socket");
        return;
    }

    unlink(IPC_PATH);

    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, IPC_PATH, sizeof(addr.sun_path) - 1);

    if(bind(wmfd, (struct sockaddr *)&addr, sizeof(addr)) < 0){
        perror("bind");
        close(wmfd);
        return;
    }

    if(listen(wmfd, 5) < 0){
        perror("listen");
        close(wmfd);
        return;
    }

    //makes the connection non-blocking
    fcntl(wmfd, F_SETFL, O_NONBLOCK);
    //close on exec
    fcntl(wmfd, F_SETFD, FD_CLOEXEC);
}

void ipc_handle(WM *wm){
    int client;
    char buf[IPC_BUFSIZE];

    char *tokens[16];
    while((client = accept(wmfd, NULL, NULL)) >= 0){
        fcntl(client, F_SETFD, FD_CLOEXEC);

        ssize_t n = read(client, buf, sizeof(buf) - 1);
        if(n > 0){
            buf[n] = '\0';
            int count = tokenize(buf, tokens);
            if(count > 0){
                interpret_tokens(wm, tokens, count);
            }
        }
        close(client);
    }

    if(errno != EAGAIN && errno != EWOULDBLOCK){
        perror("accept");
    }
}
