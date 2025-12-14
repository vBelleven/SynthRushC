#include "globals.h"
#include "ui.h"
#include "menu.h"
#include "player.h"
#include "map.h"
#include "audio.h"
#include "network.h"
#include "enemy.h"
#include "text.h"
#include "config.h"
#include "map.h"

// Definición de variable global
Language currentLang = LANG_ES;
float gHitstopTimer = 0.0f;
Vector2 gDashA = {0};
Vector2 gDashB = {0};
float gShockRingTimer = 0.0f;
float gScreenShake = 0.0f;
float gGhostTimer = 0.0f;
int   gChargeActive = 0;
Vector2 gChargeCenter = {0};

int main() {
    // 1. Inicialización
    // IMPORTANTE: Añade esta bandera para que la ventana sea redimensionable
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "SynthRush - Alpha");
    InitAudioSystem();
    InitText();
    InitSettingsDefaults(&gameSettings);
    LoadSettings("synthrush.cfg", &gameSettings);
    SetTargetFPS(60);
    // Render target for full-screen effects (chromatic aberration on dash)
    RenderTexture2D screenRT = LoadRenderTexture(GetScreenWidth(), GetScreenHeight());

    GameState state = STATE_MENU;
    GameMap map = LoadMap("level.txt");
    Player localPlayer;
    InitPlayer(&localPlayer, map.playerSpawn);
    Camera2D cam = {0};
    cam.zoom = 1.0f;
    cam.rotation = 0.0f;
    cam.offset = (Vector2){ GetScreenWidth()/2.0f, GetScreenHeight()/2.0f };
    cam.target = (Vector2){ 0, 0 };
    Vector2 prevCamTarget = cam.target;
    InitUI(); // Carga partículas
    InitProjectiles();
    InitEnemies(32);

    // 2. Bucle Principal
    while (!WindowShouldClose() && state != STATE_EXIT) {
        // --- INICIO CÓDIGO NUEVO PARA FULLSCREEN ---
        if (IsKeyPressed(KEY_F11) || (IsKeyDown(KEY_LEFT_ALT) && IsKeyPressed(KEY_ENTER))) {
            int monitor = GetCurrentMonitor();

            if (IsWindowFullscreen()) {
                // Si ya estamos en Fullscreen -> Volver a ventana 800x600
                ToggleFullscreen();
                SetWindowSize(SCREEN_WIDTH, SCREEN_HEIGHT);
                // Centrar la ventana en el monitor de nuevo para que quede bonito
                SetWindowPosition(
                    GetMonitorWidth(monitor) / 2 - SCREEN_WIDTH / 2,
                    GetMonitorHeight(monitor) / 2 - SCREEN_HEIGHT / 2
                );
            } else {
                // Si estamos en ventana -> Ir a Fullscreen nativo
                // 1. Obtener la resolución real de tu monitor
                int monitorW = GetMonitorWidth(monitor);
                int monitorH = GetMonitorHeight(monitor);
                
                // 2. Ajustar la ventana a ese tamaño
                SetWindowSize(monitorW, monitorH);
                
                // 3. Activar el modo exclusivo
                ToggleFullscreen();
            }
        }
        // --- FIN CÓDIGO NUEVO ---

        float dt = GetFrameTime();
        float rawDt = dt;
        if (gHitstopTimer > 0.0f) {
            // freeze gameplay for 1 frame duration
            dt = 0.0f;
            gHitstopTimer -= rawDt;
        }

        // --- UPDATE ---
        switch (state) {
            case STATE_MENU:
                UpdateParticles();
                // Si pulsan Jugar -> state = STATE_GAMEPLAY
                break;
            
            case STATE_GAMEPLAY:
                UpdatePlayer(&localPlayer, dt, cam, &map);
                // violent screen shake opposite to dash impulse
                if (gScreenShake > 0.0f) {
                    Vector2 shake = { (float)GetRandomValue(-2,2), (float)GetRandomValue(-2,2) };
                    cam.offset.x += shake.x * gScreenShake * 6.0f;
                    cam.offset.y += shake.y * gScreenShake * 6.0f;
                    gScreenShake *= 0.85f;
                }
                UpdateCameraSmooth(&cam, localPlayer.pos);
                Vector2 camDelta = { cam.target.x - prevCamTarget.x, cam.target.y - prevCamTarget.y };
                prevCamTarget = cam.target;
                UpdateWorldParticles();
                UpdateProjectiles(dt);
                UpdateEnemies(dt, localPlayer.pos);
                // Projectile-wall collisions: remove on hit and spawn sparks
                for (int i=0;i<128;i++) {
                    extern Projectile gProjectiles[]; // declared in player.c
                    if (!gProjectiles[i].active) continue;
                    Rectangle pr = (Rectangle){ gProjectiles[i].pos.x - 2, gProjectiles[i].pos.y - 2, 4, 4 };
                    for (int w=0; w<map.wallCount; w++) {
                        if (CheckCollisionRecs(pr, map.walls[w])) {
                            gProjectiles[i].active = 0;
                            // small cyan sparks
                            for (int s=0; s<6; s++) {
                                Vector2 jitter = { (float)GetRandomValue(-2,2), (float)GetRandomValue(-2,2) };
                                SpawnWorldParticle(gProjectiles[i].pos, NEON_CYAN, 1.2f, 2.0f);
                            }
                            break;
                        }
                    }
                }
                UpdateParticles();
                // Network: SendPlayerState(&localPlayer);
                break;

            case STATE_OPTIONS:
                UpdateParticles();
                break;
        }

        // --- DRAW ---
        BeginDrawing();
        ClearBackground(DARK_BG);

        switch (state) {
            case STATE_MENU:
                Menu_DrawMain(&state); 
                DrawParticles();
                break;

            case STATE_GAMEPLAY:
                // If window resized, ensure RT matches size
                if (screenRT.texture.width != GetScreenWidth() || screenRT.texture.height != GetScreenHeight()) {
                    UnloadRenderTexture(screenRT);
                    screenRT = LoadRenderTexture(GetScreenWidth(), GetScreenHeight());
                }

                // Render gameplay scene into RT
                BeginTextureMode(screenRT);
                ClearBackground(DARK_BG);
                // Use camera delta so background particles drift opposite to movement
                Vector2 camDeltaDraw = { cam.target.x - prevCamTarget.x, cam.target.y - prevCamTarget.y };
                DrawMapBackground(cam, camDeltaDraw);
                BeginMode2D(cam);
                DrawMapWalls(&map, cam);
                DrawEnemies(cam);
                DrawProjectiles(cam);
                DrawPlayer(&localPlayer);
                DrawWorldParticles();
                // Shock ring at dash origin
                if (gShockRingTimer > 0.0f) {
                    float t = gShockRingTimer;
                    float r = (1.0f - t) * 180.0f;
                    DrawCircleLines((int)gDashA.x, (int)gDashA.y, r, Fade(NEON_CYAN, t));
                    gShockRingTimer -= rawDt * 1.8f;
                    if (gShockRingTimer < 0.0f) gShockRingTimer = 0.0f;
                }
                // Dash trail ghosts connecting A->B (stretched silhouettes)
                if (gGhostTimer > 0.0f && Vector2Length(Vector2Subtract(gDashB, gDashA)) > 1.0f) {
                    Vector2 dir = Vector2Normalize(Vector2Subtract(gDashB, gDashA));
                    float len = Vector2Length(Vector2Subtract(gDashB, gDashA));
                    int ghosts = 7;
                    for (int i=1;i<=ghosts;i++) {
                        float f = (float)i/(float)ghosts;
                        Vector2 p = Vector2Add(gDashA, Vector2Scale(dir, len*f));
                        float a = 0.45f * (1.0f - f) * gGhostTimer;
                        // draw elongated triangle along dir
                        float rot = atan2f(dir.y, dir.x) * RAD2DEG;
                        float r = 18.0f * (1.2f - f*0.6f);
                        Vector2 v1 = { p.x + cosf(rot*DEG2RAD) * r,
                                       p.y + sinf(rot*DEG2RAD) * r };
                        Vector2 v2 = { p.x + cosf((rot+150)*DEG2RAD) * r*0.7f,
                                       p.y + sinf((rot+150)*DEG2RAD) * r*0.7f };
                        Vector2 v3 = { p.x + cosf((rot-150)*DEG2RAD) * r*0.7f,
                                       p.y + sinf((rot-150)*DEG2RAD) * r*0.7f };
                        DrawTriangleLines(v1, v2, v3, Fade(NEON_CYAN, a));
                    }
                    gGhostTimer -= rawDt * 3.0f;
                    if (gGhostTimer < 0.0f) gGhostTimer = 0.0f;
                }
                EndMode2D();
                DrawParticles();
                EndTextureMode();

                // Post-process: duplicated visuals with tinted left/right offsets during dash
                bool dashFX = (localPlayer.dashState == DASH_EXECUTE) || (localPlayer.dashState == DASH_RECOVERY);
                Rectangle src = (Rectangle){ 0, 0, (float)screenRT.texture.width, -(float)screenRT.texture.height };
                Rectangle dst = (Rectangle){ 0, 0, (float)GetScreenWidth(), (float)GetScreenHeight() };
                Vector2 origin = (Vector2){ 0, 0 };
                if (dashFX || gHitstopTimer > 0.0f) {
                    float strength = (localPlayer.dashState == DASH_EXECUTE) ? 1.0f : 0.6f;
                    float off = 6.0f * strength; // duplicate offset
                    // Base image
                    DrawTexturePro(screenRT.texture, src, dst, origin, 0.0f, Fade(WHITE, 1.0f));
                    // Left blue copy
                    Rectangle dstLeft = dst; dstLeft.x -= off;
                    DrawTexturePro(screenRT.texture, src, dstLeft, origin, 0.0f, Fade(NEON_CYAN, 0.40f));
                    // Right red copy
                    Rectangle dstRight = dst; dstRight.x += off;
                    DrawTexturePro(screenRT.texture, src, dstRight, origin, 0.0f, Fade(NEON_RED, 0.40f));
                } else {
                    DrawTexturePro(screenRT.texture, src, dst, origin, 0.0f, WHITE);
                }
                break;

            case STATE_OPTIONS:
                Menu_DrawOptions(&state);
                DrawParticles();
                break;
        }

        EndDrawing();
    }

    // 3. Limpieza
    CloseAudioSystem();
    UnloadRenderTexture(screenRT);
    SaveSettings("synthrush.cfg", &gameSettings);
    CloseWindow();

    return 0;
}