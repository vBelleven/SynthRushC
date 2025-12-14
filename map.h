#pragma once
#include "raylib.h"

#define MAP_TILE_SIZE 64
#define MAX_MAP_WALLS 2048

typedef struct {
    Rectangle walls[MAX_MAP_WALLS];
    int wallCount;
    int widthTiles;   // map width in tiles
    int heightTiles;  // map height in tiles
    Vector2 towerCenter; // from '!'
    Vector2 playerSpawn; // from '1'
} GameMap;

// Loads from a text file. Each char per tile.
// '@' -> wall 64x64; '!' -> tower center (tile center); '1' -> player spawn (tile center)
// Non-recognized characters are empty.
GameMap LoadMap(const char* filename);

// Camera helpers
void UpdateCameraSmooth(Camera2D* cam, Vector2 target);

// Background parallax drawing using camera position
// Draw a soothing particle field background that drifts opposite to player/camera movement
void DrawMapBackground(Camera2D cam, Vector2 camDelta);

// Draw map walls with neon glow
void DrawMapWalls(const GameMap* map, Camera2D cam);
