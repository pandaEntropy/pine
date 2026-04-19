#ifndef COMMANDS_H
#define COMMANDS_H

#include "forward.h"

typedef struct{
    char *name;
    int min_args;
    void (*exec)(WM *wm, char **tokens, int count);
}Command;

Command *find_cmd(char *name);

#endif
