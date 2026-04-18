#include <string.h>
#include <stdio.h>
#include <unistd.h>

#include "wm.h"
#include "commands.h"
#include "keys.h"

Command commands[] = {
    {"bind", 4, handle_bind}
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
