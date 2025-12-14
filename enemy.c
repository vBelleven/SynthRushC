#include "enemy.h"
#include <math.h>
#include <stddef.h>

static Enemy* enemies = NULL;
static int maxEnemies = 0;

void InitEnemies(int max) {
    maxEnemies = max;
    enemies = (Enemy*)MemAlloc(sizeof(Enemy) * maxEnemies);
    for (int i=0;i<maxEnemies;i++) enemies[i].active = 0;
}

int GetEnemyCount() { return maxEnemies; }
Enemy* GetEnemyArray() { return enemies; }

void SpawnEnemy(Vector2 pos) {
    for (int i=0;i<maxEnemies;i++) {
        if (!enemies[i].active) {
            enemies[i].active = 1;
            enemies[i].pos = pos;
            enemies[i].vel = (Vector2){0,0};
            enemies[i].speed = 2.6f;
            enemies[i].radius = 12.0f;
            break;
        }
    }
}

void UpdateEnemies(float dt, Vector2 playerPos) {
    // Simple pursue + separation
    for (int i=0;i<maxEnemies;i++) {
        if (!enemies[i].active) continue;
        Vector2 toTarget = { playerPos.x - enemies[i].pos.x, playerPos.y - enemies[i].pos.y };
        float len = sqrtf(toTarget.x*toTarget.x + toTarget.y*toTarget.y);
        if (len > 0.0001f) { toTarget.x /= len; toTarget.y /= len; }
        Vector2 desired = { toTarget.x * enemies[i].speed, toTarget.y * enemies[i].speed };
        Vector2 steer = { (desired.x - enemies[i].vel.x) * 0.1f, (desired.y - enemies[i].vel.y) * 0.1f };
        enemies[i].vel.x += steer.x;
        enemies[i].vel.y += steer.y;
        // Separation
        Vector2 sep = (Vector2){0,0};
        for (int j=0;j<maxEnemies;j++) {
            if (i==j || !enemies[j].active) continue;
            Vector2 d = { enemies[i].pos.x - enemies[j].pos.x, enemies[i].pos.y - enemies[j].pos.y };
            float dist = sqrtf(d.x*d.x + d.y*d.y);
            if (dist < 25 && dist > 0.001f) {
                sep.x += d.x * (1.0f/(dist*dist));
                sep.y += d.y * (1.0f/(dist*dist));
            }
        }
        enemies[i].vel.x += sep.x * 1.5f;
        enemies[i].vel.y += sep.y * 1.5f;
        enemies[i].pos.x += enemies[i].vel.x;
        enemies[i].pos.y += enemies[i].vel.y;
    }
}

void DrawEnemies(Camera2D cam) {
    (void)cam;
    for (int i=0;i<maxEnemies;i++) {
        if (!enemies[i].active) continue;
        // Neon square
        Rectangle r = { enemies[i].pos.x - enemies[i].radius, enemies[i].pos.y - enemies[i].radius, enemies[i].radius*2, enemies[i].radius*2 };
        DrawRectangleLinesEx(r, 2.0f, NEON_RED);
        Rectangle r2 = { r.x+1, r.y+1, r.width-2, r.height-2 };
        DrawRectangleLinesEx(r2, 2.0f, Fade(NEON_PURPLE, 0.7f));
    }
}
