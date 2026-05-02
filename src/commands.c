#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include "wm.h"
#include "commands.h"
#include "layout.h"
#include "config.h"

void refresh_state(WM *wm){
    for(Client *c = wm->clients; c; c = c->next){
        if(c == wm->workspaces[wm->current_ws].focused){
            XSetWindowBorder(wm->dpy, c->parent, wm->config.active_border_color);
        }
        else if(c->wtags & wm->current_wtag){
            XSetWindowBorder(wm->dpy, c->parent, wm->config.inactive_border_color);
        }

        if(c->parent) 
            XSetWindowBorderWidth(wm->dpy, c->parent, wm->config.border_width);
    }

    tile(wm);
}

unsigned int parse_color(char *hexstr){
    if(!hexstr || hexstr[0] != '#'){
        fprintf(stderr, "parse_color: Invalid color\n");
        return 0;
    }

    long int hex = strtol(hexstr + 1, NULL, 16);
    return (unsigned long)hex;
}

int parse_dir_string(char *dirstr){
    if(strcmp(dirstr, "right") == 0){
        return DIR_RIGHT;
    }
    else if(strcmp(dirstr, "left") == 0){
        return DIR_LEFT;
    }
    else if(strcmp(dirstr, "up") == 0){
        return DIR_UP;
    }
    else if(strcmp(dirstr, "down") == 0){
        return DIR_DOWN;
    }

    fprintf(stderr, "parse_dir_string: invalid direction\n");
    return -1;
}

void set_active_border_color(WM *wm, char **tokens, int count){
    wm->config.active_border_color = parse_color(tokens[1]);
    refresh_state(wm);
}

void set_inactive_border_color(WM *wm, char **tokens, int count){
    wm->config.inactive_border_color = parse_color(tokens[1]);
    refresh_state(wm);
}

void set_border_width(WM *wm, char **tokens, int count){
    wm->config.border_width = atoi(tokens[1]);
    refresh_state(wm);
}

//ex: focus left
void exec_focus(WM *wm, char **tokens, int count){
    int dir = parse_dir_string(tokens[1]);
    if(dir == -1) return;

    if(!wm->workspaces[wm->current_ws].focused) 
        return;

    wm->workspaces[wm->current_ws].layout->focus(wm, dir);
}

//ex: rotate
void exec_rotate(WM *wm, char **tokens, int count){
    wm->workspaces[wm->current_ws].layout->rotate(wm);
}

//ex: hide_all
void exec_unmap(WM *wm, char **tokens, int count){
    unmap(wm);
}

//ex: kill_window
void exec_kill(WM *wm, char **tokens, int count){
    if(!wm->workspaces[wm->current_ws].focused) return;

    kill_client(wm, wm->workspaces[wm->current_ws].focused);
}
//ex: resize left
void exec_resize(WM *wm,  char **tokens, int count){
    int dir = parse_dir_string(tokens[1]);
    if(dir == -1) return;

    resize(wm, dir);
}

//ex: set_master
void exec_set_master(WM *wm,char **tokens, int count){
    set_master(wm);
}

//ex: cycle_layout
void exec_cycle_layout(WM *wm, char **tokens, int count){
    cycle_layout(wm);
}

//ex: switch_workspace right
void exec_switch_workspace(WM *wm, char **tokens, int count){
    int dir = parse_dir_string(tokens[1]);
    if(dir == -1) return;

    switch_workspace(wm, dir);
}

//es: reload_config
void exec_conf_reload(WM *wm, char **tokens, int count){
    reload_config(wm);
}

//ex: move_client_ws 3
void exec_move_cli_ws(WM *wm, char **tokens, int count){
    int index = atoi(tokens[1]);

    move_cli_ws(wm, index);
}

Command commands[] = {
    {"set_active_border_color", 2,  set_active_border_color},
    {"set_inactive_border_color", 2, set_inactive_border_color},
    {"set_border_width", 2, set_border_width},
    {"focus", 2, exec_focus},
    {"rotate", 1, exec_rotate},
    {"hide_all", 1, exec_unmap},
    {"kill_window", 1, exec_kill},
    {"resize", 2, exec_resize},
    {"set_master", 1, exec_set_master},
    {"cycle_layout", 1, exec_cycle_layout},
    {"switch_workspace", 2, exec_switch_workspace},
    {"reload_config", 1, exec_conf_reload},
    {"move_client_ws", 2, exec_move_cli_ws}
};

Command *find_cmd(char *name){
    for(int i = 0; i < sizeof(commands) / sizeof(Command); i++){
        if(strcmp(name, commands[i].name) == 0){
            return &commands[i];
        }
    }
    fprintf(stderr, "find_cmd: Command - %s - not found\n", name);
    return NULL;
}
