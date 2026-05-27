#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#define IPC_PATH "/tmp/wm.sock"
#define BUFSIZE 128

int main(int argc, char **argv){
    if(argc < 2){
        fprintf(stderr, "pinemsg: Insufficient argument amount\n");
        return 1;
    }

    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if(fd < 0){
        perror("socket");
        return 1;
    }

    struct sockaddr_un addr = {0};
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, IPC_PATH, sizeof(addr.sun_path) - 1);

    if(connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0){
        perror("connect");
        close(fd);
        return 1;
    }
    for(int i = 1; i < argc; i++){
        write(fd, argv[i], strlen(argv[i]));

        if(i < argc - 1)
            write(fd, " ", 1);
    }

    char buf[BUFSIZE];
    ssize_t n;

    while((n = read(fd, buf, sizeof(buf))) > 0){
        printf("%s", buf);
    }

    close(fd);
}
