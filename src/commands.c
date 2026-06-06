#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

#include "pine.h"
#include "commands.h"
#include "layout.h"


#define MAX_TOK 16

void refresh_state(WM *wm){
    for(Client *c = wm->clients; c; c = c->next){
        if(c == wm->workspaces[wm->current_ws].focused){
            XSetWindowBorder(wm->dpy, c->win, wm->config.active_border_color);
        }
        else if(c->wtags & wm->current_wtag){
            XSetWindowBorder(wm->dpy, c->win, wm->config.inactive_border_color);
        }

        XSetWindowBorderWidth(wm->dpy, c->win, wm->config.border_width);
    }

    tile(wm);
}

unsigned int parse_color(char *hexstr){
    if(!hexstr || hexstr[0] != '#'){
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

    return -1;
}

void set_active_border_color(WM *wm, char **tokens, int count){
    (void) count;
    wm->config.active_border_color = parse_color(tokens[1]);
    refresh_state(wm);
}

void set_inactive_border_color(WM *wm, char **tokens, int count){
    (void) count;
    wm->config.inactive_border_color = parse_color(tokens[1]);
    refresh_state(wm);
}

void set_border_width(WM *wm, char **tokens, int count){
    (void) count;
    wm->config.border_width = atoi(tokens[1]);
    refresh_state(wm);
}

//ex: focus left
void exec_focus(WM *wm, char **tokens, int count){
    (void) count;
    int dir = parse_dir_string(tokens[1]);
    if(dir == -1) return;

    if(!wm->workspaces[wm->current_ws].focused) 
        return;

    wm->workspaces[wm->current_ws].layout->focus(wm, dir);
}

//ex: rotate
void exec_rotate(WM *wm, char **tokens, int count){
    (void) count;
    (void) tokens;
    wm->workspaces[wm->current_ws].layout->rotate(wm);
}

//ex: hide_all
void exec_unmap(WM *wm, char **tokens, int count){
    (void) tokens;
    (void) count;
    unmap(wm);
}

//ex: kill_window
void exec_kill(WM *wm, char **tokens, int count){
    (void) tokens;
    (void) count;
    if(!wm->workspaces[wm->current_ws].focused) return;

    kill_client(wm, wm->workspaces[wm->current_ws].focused);
}
//ex: resize left
void exec_resize(WM *wm,  char **tokens, int count){
    (void) count;
    int dir = parse_dir_string(tokens[1]);
    if(dir == -1) return;

    resize(wm, dir);
}

//ex: set_master
void exec_set_master(WM *wm,char **tokens, int count){
    (void) tokens;
    (void) count;
    set_master(wm);
}

//ex: cycle_layout
void exec_cycle_layout(WM *wm, char **tokens, int count){
    (void) tokens;
    (void) count;
    cycle_layout(wm);
}

//ex: switch_workspace right
void exec_switch_workspace(WM *wm, char **tokens, int count){
    (void) count;
    int dir = parse_dir_string(tokens[1]);
    if(dir == -1) return;

    switch_workspace(wm, dir);
}

//ex: reload_config
void exec_conf_reload(WM *wm, char **tokens, int count){
    (void) count;
    exec_config(wm, tokens[1]);
    refresh_state(wm);
}

//ex: move_client_ws 3
void exec_move_cli_ws(WM *wm, char **tokens, int count){
    (void) count;
    int index = atoi(tokens[1]);

    move_cli_ws(wm, index);
}

void set_active_log_level(WM *wm, char **tokens, int count){
    (void)count;
    char *levels[] = {"DEBUG", "INFO", "WARN", "ERROR"};
    for(size_t i = 0; i < sizeof(levels) / sizeof(char*); i++){
        if(strcmp(levels[i], tokens[1]) == 0){
            wm->current_log_level = i;
            return;
        }
    }
}

void set_gap_size(WM *wm, char **tokens, int count){
    (void)count;
    int diff = atoi(tokens[1]) - wm->config.gap_size;

    wm->usable_width -= 2 * diff;
    wm->usable_height -= 2 * diff;
    wm->usable_x += diff;
    wm->usable_y += diff;
    wm->config.gap_size = atoi(tokens[1]);

    refresh_state(wm);
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
    {"move_client_ws", 2, exec_move_cli_ws},
    {"set_log_level", 2, set_active_log_level},
    {"set_gap_size", 2, set_gap_size}
};

Command *find_cmd(WM *wm, char *name){
    for(long unsigned int i = 0; i < sizeof(commands) / sizeof(Command); i++){
        if(strcmp(name, commands[i].name) == 0){
            return &commands[i];
        }
    }
    level_log(wm, WARN, "Command - %s - not found", name);
    return NULL;
}

int interpret_tokens(WM *wm, char **tokens, int count){
    if(count == 0) return 1;

    Command *cmd = find_cmd(wm, tokens[0]);
    if(!cmd) return 1;

    if(count < cmd->min_args){
        level_log(wm, WARN, "Missing arguments");
        return 1;
    }

    cmd->exec(wm, tokens, count);

    return 0;
}

int tokenize(char *line, char **tokens){
    int count = 0;

    char *token = strtok(line, " \t\n\0");
    while(token && count < MAX_TOK){
        tokens[count] = token;
        count++;
        token = strtok(NULL, " \t\n\0");
    }

    return count;
}
