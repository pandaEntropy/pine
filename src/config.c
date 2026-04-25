#include <stdio.h>
#include <string.h>
#include <X11/Xlib.h>

#include "commands.h"
#include "layout.h"
#include "wm.h"

#define MAX_TOK 16

int tokenize(char *line, char **tokens){
    int count = 0;

    char *token = strtok(line, " \t\n");
    while(token && count < MAX_TOK){
        tokens[count] = token;
        count++;
        token = strtok(NULL, " \t\n");
    }

    return count;
}

void interpret_tokens(WM *wm, char **tokens, int count){
    if(count == 0) return;

    Command *cmd = find_cmd(tokens[0]);
    if(!cmd) return;

    if(count < cmd->min_args){
        fprintf(stderr, "interpret: Insufficient amount of arguments\n");
        return;
    }

    cmd->exec(wm, tokens, count);
}

void load_config(WM *wm, char *path){
    FILE *file = fopen(path, "r");
    if(!file){
        perror("fopen");
        return;
    }

    char line[256];
    char *tokens[MAX_TOK];
    while(fgets(line, sizeof(line), file)){
        if(line[0] == '#' || line[0] == '\n')
            continue;

        int count = tokenize(line, tokens);
        interpret_tokens(wm, tokens, count);
    }

    fclose(file);
}

void reload_config(WM *wm, Arg *arg){
    load_config(wm, arg->str);

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
