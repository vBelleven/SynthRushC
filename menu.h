#ifndef MENU_H
#define MENU_H
#include "globals.h"

// Menu draw wrappers (implemented in menu.c, call into UI)
void Menu_DrawMain(GameState* state);
void Menu_DrawOptions(GameState* state);

#endif
