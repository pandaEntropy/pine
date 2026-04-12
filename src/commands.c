#include <string.h>
#include <stdio.h>
#include <unistd.h>

#include "wm.h"
#include "commands.h"
#include "layout.h"

typedef struct{
    char *name;
    void (*func)(WM *wm, const Arg *);
    Arg arg;
}Command;

const char *termcmd[] = {"xterm", NULL};

void spawn(WM *wm, const Arg *arg){
    (void)wm;
    if(fork() == 0){
        setsid(); // Create a new session for the child
        execvp(arg->cparr[0], (char *const *)arg->cparr);

        //If child fails
        perror("execvp failed");
        _exit(1); // A safe way to terminate the forked child
    }
}

Command commands[] = {
    {"focus_right", cmd_focus, {.i = DIR_RIGHT}},
    {"focus_left", cmd_focus, {.i = DIR_LEFT}},
    {"focus_down", cmd_focus, {.i = DIR_DOWN}},
    {"focus_up", cmd_focus, {.i = DIR_UP}},
    {"kill", cmd_kill, {0}},
    {"spawn", spawn, {.cparr = termcmd}},
    {"set_master", set_master, {0}},
    {"rotate", rotate, {0}},
    {"unmap", unmap, {0}},
    {"resize_right", resize, {.i = DIR_RIGHT}},
    {"resize_left", resize, {.i = DIR_LEFT}},
    {"resize_up", resize, {.i = DIR_UP}},
    {"resize_down", resize, {.i = DIR_DOWN}},
    {"switch_layout", switch_layout, {0}},
    {"switch_workspace_left", switch_workspace, {.i = DIR_LEFT}},
    {"switch_workspace_right", switch_workspace, {.i = DIR_RIGHT}},
    {"switch_cli_ws_1", switch_cli_ws, {.i = 0}},
    {"switch_cli_ws_2", switch_cli_ws, {.i = 1}},
    {"switch_cli_ws_3", switch_cli_ws, {.i = 2}},
    {"switch_cli_ws_4", switch_cli_ws, {.i = 3}},
    {"switch_cli_ws_5", switch_cli_ws, {.i = 4}},
    {"switch_cli_ws_6", switch_cli_ws, {.i = 5}},
    {"switch_cli_ws_7", switch_cli_ws, {.i = 6}},
    {"switch_cli_ws_8", switch_cli_ws, {.i = 7}},
    {"switch_cli_ws_9", switch_cli_ws, {.i = 8}}
};

void dispatch_command(WM *wm, const char *cmd){
    for(size_t i = 0; i < (sizeof(commands) / sizeof(Command)); i++){
        if(strcmp(commands[i].name, cmd) == 0){
            commands[i].func(wm, &commands[i].arg);
            return;
        }
    }
    fprintf(stderr, "WARN: Command not found");
}
