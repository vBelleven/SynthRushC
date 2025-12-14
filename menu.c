#include "menu.h"
#include "ui.h"

// Provide unique wrapper names to avoid recursion and symbol overlap
static void UI_DrawMainMenu(GameState* state) { DrawMainMenu(state); }
static void UI_DrawOptionsMenu(GameState* state) { DrawOptionsMenu(state); }

void Menu_DrawMain(GameState* state) { UI_DrawMainMenu(state); }
void Menu_DrawOptions(GameState* state) { UI_DrawOptionsMenu(state); }
