#ifndef CONFIG_H
#define CONFIG_H

#include "forward.h"

void load_config(WM *wm);

int tokenize(char *line, char **tokens);

void interpret_tokens(WM *wm, char **tokens, int count);

void reload_config(WM *wm);

#endif
