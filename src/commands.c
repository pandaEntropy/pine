#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include "wm.h"
#include "commands.h"
#include "keys.h"

unsigned int parse_color(char *hexstr){
    if(!hexstr || hexstr[0] != '#'){
        fprintf(stderr, "parse_color: Invalid color\n");
        return 0;
    }

    long int hex = strtol(hexstr + 1, NULL, 16);
    return (unsigned long)hex;
}

void handle_bind(WM *wm, char **tokens, int count){
    char *name = tokens[1];

    KeySym sym = XStringToKeysym(tokens[2]);
    if(sym == NoSymbol){
        fprintf(stderr, "handle_bind: KeySym not found\n");
        return;
    }

    unsigned int modmask = 0;
    for(int i = 3; i < count; i++){
        modmask |= parse_mod(tokens[i]);
    }

    bind_key(wm, name, sym, modmask);
}

void set_active_border_color(WM *wm, char **tokens, int count){
    wm->config.active_border_color = parse_color(tokens[1]);
}

void set_inactive_border_color(WM *wm, char **tokens, int count){
    wm->config.inactive_border_color = parse_color(tokens[1]);
}

void set_border_width(WM *wm, char **tokens, int count){
    wm->config.border_width = atoi(tokens[1]);
}

Command commands[] = {
    {"bind", 4, handle_bind},
    {"set_active_border_color", 2,  set_active_border_color},
    {"set_inactive_border_color", 2, set_inactive_border_color},
    {"set_border_width", 2, set_border_width}
};

Command *find_cmd(char *name){
    for(int i = 0; i < sizeof(commands) / sizeof(Command); i++){
        if(strcmp(name, commands[i].name) == 0){
            return &commands[i];
        }
    }
    fprintf(stderr, "find_cmd: Command not found\n");
    return NULL;
}
