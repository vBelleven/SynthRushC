#ifndef UI_H
#define UI_H
#include "globals.h"

void InitUI();
void UpdateParticles();
void DrawParticles();
void SpawnExplosion(Vector2 pos, Color color);
void SpawnParticle(Vector2 pos, Color color, float speedMult, float sizeBase);
// World-space particles (draw inside BeginMode2D)
void SpawnWorldParticle(Vector2 pos, Color color, float speedMult, float sizeBase);
void SpawnWorldExplosion(Vector2 pos, Color color);
void UpdateWorldParticles();
void DrawWorldParticles();
bool DrawJuicyButton(Rectangle rect, int textIndex, int fontSize);

// --- ESTA ES LA LÍNEA CORREGIDA ---
void DrawMainMenu(GameState* currentState); 
// ----------------------------------

void DrawOptionsMenu(GameState* currentState);

#endif