#ifndef COMMANDS_H
#define COMMANDS_H

#include "forward.h"

typedef struct{
    char *name;
    int min_args;
    void (*exec)(WM *wm, char **tokens, int count);
}Command;

Command *find_cmd(WM *wm, char *name);

void refresh_state(WM *wm);

int tokenize(char *line, char **tokens);

int interpret_tokens(WM *wm, char **tokens, int count);

#endif
