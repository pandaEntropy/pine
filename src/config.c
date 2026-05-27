#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <X11/Xlib.h>

#include "commands.h"
#include "pine.h"
#include "config.h"

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

    Command *cmd = find_cmd(wm, tokens[0]);
    if(!cmd) return;

    if(count < cmd->min_args){
        level_log(wm, WARN, "insufficient amount of arguments in config file");
        return;
    }

    cmd->exec(wm, tokens, count);
}

void load_config(WM *wm){
    FILE *file = fopen(wm->config.conf_addr, "r");
    if(!file){
        level_log(wm, WARN, "failed to load config");
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
    load_config(wm);

    refresh_state(wm);
    level_log(wm, DEBUG, "config successfully reloaded");
}
