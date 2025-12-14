#ifndef ENEMY_H
#define ENEMY_H
#include "raylib.h"
#include "globals.h"

typedef struct {
    Vector2 pos;
    Vector2 vel;
    float speed;
    float radius;
    int active;
} Enemy;

void InitEnemies(int max);
void SpawnEnemy(Vector2 pos);
void UpdateEnemies(float dt, Vector2 playerPos);
void DrawEnemies(Camera2D cam);
int GetEnemyCount();
Enemy* GetEnemyArray();

#endif
