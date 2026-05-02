#include <stdio.h>
#include <string.h>
#include <X11/Xlib.h>

#include "commands.h"
#include "pine.h"

#define MAX_TOK 16

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

void reload_config(WM *wm){
    load_config(wm, wm->config.conf_addr);

    refresh_state(wm);
}
