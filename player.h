#ifndef PLAYER_H
#define PLAYER_H
#include "raylib.h"
#include "raymath.h"
#include "globals.h"
#include "map.h"

void InitPlayer(Player* p, Vector2 spawn);
void UpdatePlayer(Player* p, float dt, Camera2D cam, const GameMap* map);
void DrawPlayer(const Player* p);

// Simple projectile system to mirror test.py shooting
typedef struct {
	Vector2 pos;
	Vector2 vel;
	float life;
	int active;
} Projectile;

void InitProjectiles();
void SpawnProjectile(Vector2 pos, Vector2 vel);
void UpdateProjectiles(float dt);
void DrawProjectiles(Camera2D cam);
extern Projectile gProjectiles[128];

#endif