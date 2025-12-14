#include "map.h"
#include "globals.h"
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>

static Vector2 TileCenter(int tx, int ty) {
    return (Vector2){ tx*MAP_TILE_SIZE + MAP_TILE_SIZE/2.0f, ty*MAP_TILE_SIZE + MAP_TILE_SIZE/2.0f };
}

GameMap LoadMap(const char* filename) {
    GameMap map = {0};
    map.wallCount = 0;
    map.widthTiles = 0;
    map.heightTiles = 0;
    map.towerCenter = (Vector2){0,0};
    map.playerSpawn = (Vector2){MAP_TILE_SIZE*2.0f, MAP_TILE_SIZE*2.0f};

    FILE* f = fopen(filename, "rb");
    if (!f) {
        // Fallback: a small empty map
        map.widthTiles = 20;
        map.heightTiles = 12;
        return map;
    }

    char line[1024];
    int ty = 0;
    while (fgets(line, sizeof(line), f)) {
        // Trim newline
        size_t len = strlen(line);
        while (len>0 && (line[len-1]=='\n' || line[len-1]=='\r')) { line[--len] = '\0'; }
        if ((int)len > map.widthTiles) map.widthTiles = (int)len;
        for (int tx=0; tx<(int)len; tx++) {
            char c = line[tx];
            if (c == '@') {
                if (map.wallCount < MAX_MAP_WALLS) {
                    Rectangle r = { tx*MAP_TILE_SIZE, ty*MAP_TILE_SIZE, MAP_TILE_SIZE, MAP_TILE_SIZE };
                    map.walls[map.wallCount++] = r;
                }
            } else if (c == '!') {
                map.towerCenter = TileCenter(tx, ty);
            } else if (c == '1') {
                map.playerSpawn = TileCenter(tx, ty);
            }
        }
        ty++;
    }
    fclose(f);
    map.heightTiles = ty;
    return map;
}

void UpdateCameraSmooth(Camera2D* cam, Vector2 target) {
    // Center the camera on target regardless of resolution
    Vector2 screenCenter = (Vector2){ GetScreenWidth()/2.0f, GetScreenHeight()/2.0f };
    float s = 0.10f; // smoothing factor
    cam->target.x = cam->target.x + (target.x - cam->target.x) * s;
    cam->target.y = cam->target.y + (target.y - cam->target.y) * s;
    cam->offset = screenCenter; // keeps target visually centered
}

static void DrawGlowRectLines(Rectangle r, Color base, int layers) {
    for (int i=0;i<layers;i++) {
        float alpha = 0.10f + i * 0.05f;
        Color c = Fade(base, alpha);
        DrawRectangleLines((int)r.x, (int)r.y, (int)r.width, (int)r.height, c);
    }
}

void DrawMapBackground(Camera2D cam, Vector2 camDelta) {
    // Soothing particle field reacting to camera movement (opposite drift)
    int scrW = GetScreenWidth();
    int scrH = GetScreenHeight();
    float time = (float)GetTime();

    typedef struct { Vector2 pos; float speed; float size; Color col; } BgParticle;
    static BgParticle* field = NULL;
    static int count = 0;
    static int lastW = 0, lastH = 0;

    if (!field || lastW != scrW || lastH != scrH) {
        if (field) { free(field); field = NULL; }
        lastW = scrW; lastH = scrH;
        count = (scrW * scrH) / 2200; // density scales with resolution
        if (count < 300) count = 300;
        if (count > 1400) count = 1400;
        field = (BgParticle*)malloc(sizeof(BgParticle) * count);
        for (int i=0;i<count;i++) {
            field[i].pos = (Vector2){ (float)(GetRandomValue(0, scrW-1)), (float)(GetRandomValue(0, scrH-1)) };
            field[i].speed = 0.2f + (GetRandomValue(0, 100) / 100.0f) * 1.0f; // slow soothing drift
            field[i].size = 1.0f + (GetRandomValue(0, 100) / 100.0f) * 2.0f;
            float huePick = GetRandomValue(0, 100);
            Color base = (huePick < 50) ? Fade(WHITE, 0.85f) : Fade(NEON_CYAN, 0.70f);
            field[i].col = base;
        }
    }

    // Update positions: gentle drift + opposite camera movement
    Vector2 drift = { 0.0f, 10.0f * sinf(time * 0.25f) }; // subtle vertical oscillation
    Vector2 oppose = { -camDelta.x * 0.6f, -camDelta.y * 0.6f };
    for (int i=0;i<count;i++) {
        field[i].pos.x += oppose.x + drift.x * field[i].speed * 0.02f;
        field[i].pos.y += oppose.y + drift.y * field[i].speed * 0.02f;
        // wrap-around
        if (field[i].pos.x < 0) field[i].pos.x += scrW;
        if (field[i].pos.x >= scrW) field[i].pos.x -= scrW;
        if (field[i].pos.y < 0) field[i].pos.y += scrH;
        if (field[i].pos.y >= scrH) field[i].pos.y -= scrH;
    }

    // Draw particles: tiny soft dots with subtle twinkle
    BeginBlendMode(BLEND_ALPHA);
    for (int i=0;i<count;i++) {
        float tw = 0.75f + 0.25f * sinf(time * (0.6f + 0.2f*field[i].speed) + i*0.015f);
        Color c = Fade(field[i].col, tw);
        int sz = (int)field[i].size;
        DrawRectangle((int)field[i].pos.x, (int)field[i].pos.y, sz, sz, c);
    }
    EndBlendMode();
}

void DrawMapWalls(const GameMap* map, Camera2D cam) {
    (void)cam; // not needed directly; camera applied by BeginMode2D outside
    for (int i=0;i<map->wallCount;i++) {
        Rectangle r = map->walls[i];
        // interior oscuro semitransparente
        DrawRectangleRec(r, Fade(DARK_BG, 0.6f));
        // borde de 2-3px
        DrawRectangleLines((int)r.x, (int)r.y, (int)r.width, (int)r.height, Fade(NEON_CYAN, 0.9f));
        Rectangle r2 = { r.x+1, r.y+1, r.width-2, r.height-2 };
        DrawRectangleLines((int)r2.x, (int)r2.y, (int)r2.width, (int)r2.height, Fade(NEON_PURPLE, 0.6f));
        // halo suave
        DrawGlowRectLines(r, NEON_PURPLE, 6);
    }
}
