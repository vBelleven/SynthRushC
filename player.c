#include "player.h"
// --- Projectiles ---
#define MAX_PROJECTILES 128
Projectile gProjectiles[MAX_PROJECTILES];

void InitProjectiles() {
    for (int i=0;i<MAX_PROJECTILES;i++) gProjectiles[i].active = 0;
}

void SpawnProjectile(Vector2 pos, Vector2 vel) {
    for (int i=0;i<MAX_PROJECTILES;i++) {
        if (!gProjectiles[i].active) {
            gProjectiles[i].active = 1;
            gProjectiles[i].pos = pos;
            gProjectiles[i].vel = vel;
            gProjectiles[i].life = 2.0f; // seconds
            break;
        }
    }
}

void UpdateProjectiles(float dt) {
    extern GameMap gCurrentMap; // optional: if exposed globally; else handled in main
    for (int i=0;i<MAX_PROJECTILES;i++) {
        if (!gProjectiles[i].active) continue;
        gProjectiles[i].pos.x += gProjectiles[i].vel.x;
        gProjectiles[i].pos.y += gProjectiles[i].vel.y;
        gProjectiles[i].life -= dt;
        // Wall collision check (AABB rect hit test)
        Rectangle pr = { gProjectiles[i].pos.x - 2, gProjectiles[i].pos.y - 2, 4, 4 };
        // We don't have global map here; collision will be handled in main integration step.
        if (gProjectiles[i].life <= 0) gProjectiles[i].active = 0;
    }
}

void DrawProjectiles(Camera2D cam) {
    (void)cam;
    for (int i=0;i<MAX_PROJECTILES;i++) {
        if (!gProjectiles[i].active) continue;
        Vector2 start = gProjectiles[i].pos;
        Vector2 end = { start.x - gProjectiles[i].vel.x*2.0f, start.y - gProjectiles[i].vel.y*2.0f };
        DrawLineEx(start, end, 3.0f, NEON_CYAN);
        DrawCircleV(start, 2.0f, NEON_WHITE);
    }
}
#include "ui.h" // efectos de partículas
#include "audio.h"
#include <math.h>

// Constantes de movimiento
#define ACCEL_FORCE 600.0f
#define MAX_SPEED   500.0f

// Dash
#define DASH_CHARGE_TIME   0.10f
#define DASH_RECOVERY_TIME 0.30f
#define DASH_DISTANCE      260.0f

// Raycast simple contra rectángulos del mapa para recortar el destino
static Vector2 RaycastClampDestination(Vector2 origin, Vector2 dest, const GameMap* map) {
    Vector2 dir = Vector2Subtract(dest, origin);
    float len = Vector2Length(dir);
    if (len <= 0.0001f) return origin;
    dir = Vector2Scale(dir, 1.0f/len);
    float step = 8.0f;
    Vector2 p = origin;
    for (float t=0.0f; t<=len; t+=step) {
        p = Vector2Add(origin, Vector2Scale(dir, t));
        // chequear colisión con paredes (punto dentro del rect)
        for (int i=0;i<map->wallCount;i++) {
            Rectangle r = map->walls[i];
            if (CheckCollisionPointRec(p, r)) {
                // retroceder un poco y retornar
                return Vector2Add(p, Vector2Scale(dir, -step));
            }
        }
    }
    return dest;
}

void InitPlayer(Player* p, Vector2 spawn) {
    p->pos = spawn;
    p->vel = (Vector2){0,0};
    p->acc = (Vector2){0,0};
    p->rotation = 0.0f;
    p->radius = 20.0f;
    p->dashState = DASH_IDLE;
    p->dashTimer = 0.0f;
    p->dashCooldown = 0.0f;
    p->drawScale = (Vector2){1.0f, 1.0f};
    // Inicializar ring buffer del trail
    p->trailHead = 0;
    for (int i=0;i<TRAIL_LENGTH;i++) { p->trailPos[i] = spawn; p->trailAngle[i] = 0.0f; }
    p->color = NEON_RED;
}

void UpdatePlayer(Player* p, float dt, Camera2D cam, const GameMap* map) {
    // Objetivo: dirección hacia el mouse en mundo
    Vector2 mouseWorld = GetScreenToWorld2D(GetMousePosition(), cam);
    Vector2 heading = Vector2Normalize(Vector2Subtract(mouseWorld, p->pos));
    p->rotation = atan2f(heading.y, heading.x) * RAD2DEG;

    // Física Newtoniana: suma de fuerzas
    Vector2 force = (Vector2){0,0};
    if (IsKeyDown(KEY_W)) force = Vector2Add(force, Vector2Scale(heading, ACCEL_FORCE));
    if (IsKeyDown(KEY_S)) force = Vector2Add(force, Vector2Scale(heading, -ACCEL_FORCE * 0.7f));

    // Integración (Newtoniana)
    p->acc = force;
    p->vel = Vector2Add(p->vel, Vector2Scale(p->acc, dt));
    // Fricción: más cercana a Python (0.94)
    p->vel = Vector2Scale(p->vel, 0.94f);
    // Limitar velocidad
    float vlen = Vector2Length(p->vel);
    if (vlen > MAX_SPEED) p->vel = Vector2Scale(Vector2Normalize(p->vel), MAX_SPEED);

    // Dash input
    if (IsKeyPressed(KEY_E) && p->dashCooldown <= 0.0f && p->dashState == DASH_IDLE) {
        p->dashState = DASH_CHARGE;
        p->dashTimer = DASH_CHARGE_TIME;
        PlayUIClick();
    }

    // Máquina de estados de dash
    switch (p->dashState) {
        case DASH_IDLE: {
                        // Shooting (left mouse)
                        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
                            Vector2 aim = { cosf(p->rotation*DEG2RAD), sinf(p->rotation*DEG2RAD) };
                            SpawnProjectile(p->pos, Vector2Scale(aim, 18.0f));
                        }
            // movimiento normal
            p->pos = Vector2Add(p->pos, Vector2Scale(p->vel, dt));
            // drawScale hacia (1,1)
            p->drawScale.x = Lerp(p->drawScale.x, 1.0f, 0.2f);
            p->drawScale.y = Lerp(p->drawScale.y, 1.0f, 0.2f);
        } break;
        case DASH_CHARGE: {
            p->dashTimer -= dt;
            // Reducir levemente la velocidad, manteniendo inercia
            p->vel = Vector2Scale(p->vel, 0.95f);
            // Squash en la dirección del movimiento (aplastado)
            float squash = 0.45f; // más aplastado
            p->drawScale.x = Lerp(p->drawScale.x, squash, 0.55f);
            p->drawScale.y = Lerp(p->drawScale.y, 1.65f, 0.55f);
            // activar succión de luz
            extern int gChargeActive; extern Vector2 gChargeCenter;
            gChargeActive = 1; gChargeCenter = p->pos;
            if (p->dashTimer <= 0.0f) {
                p->dashState = DASH_EXECUTE;
                gChargeActive = 0;
            }
        } break;
        case DASH_EXECUTE: {
            // Calcular destino y teletransportar un frame
            Vector2 dest = Vector2Add(p->pos, Vector2Scale(heading, DASH_DISTANCE));
            dest = RaycastClampDestination(p->pos, dest, map);
            // Registrar FX globals
            extern Vector2 gDashA; extern Vector2 gDashB; extern float gShockRingTimer; extern float gHitstopTimer; extern float gScreenShake;
            extern float gGhostTimer;
            gDashA = p->pos;
            gDashB = dest;
            gShockRingTimer = 0.15f;
            gHitstopTimer = 1.0f/60.0f; // 1 frame hitstop
            gScreenShake = 1.0f;
            gGhostTimer = 0.4f;
            // ondas de choque en origen (world-space)
            SpawnWorldExplosion(p->pos, NEON_CYAN);
            p->pos = dest;
            // Impulso: gran aceleración temporal que luego decae naturalmente
            p->vel = Vector2Add(p->vel, Vector2Scale(heading, MAX_SPEED*1.0f));
            // A continuación, entrar en recuperación
            p->dashState = DASH_RECOVERY;
            p->dashTimer = DASH_RECOVERY_TIME;
            p->dashCooldown = 0.6f; // cooldown general
            // Stretch al salir
            p->drawScale.x = 1.7f;
            p->drawScale.y = 0.45f;
        } break;
        case DASH_RECOVERY: {
            p->dashTimer -= dt;
            // Mantener la inercia: el drift global ya aplica fricción suave (0.98)
            p->pos = Vector2Add(p->pos, Vector2Scale(p->vel, dt));
            // Volver a (1,1) gradualmente
            p->drawScale.x = Lerp(p->drawScale.x, 1.0f, 0.35f);
            p->drawScale.y = Lerp(p->drawScale.y, 1.0f, 0.35f);
            // Chispas intensas durante el derrape
            Vector2 back = Vector2Add(p->pos, Vector2Scale(heading, -p->radius - 8.0f));
            for (int i=0;i<3;i++) SpawnWorldParticle(back, NEON_CYAN, 2.0f, 2.5f);
            if (p->dashTimer <= 0.0f) {
                p->dashState = DASH_IDLE;
            }
        } break;
    }

    if (p->dashCooldown > 0.0f) p->dashCooldown -= dt;

    // Trail: actualizar ring buffer con posición y ángulo
    p->trailHead = (p->trailHead + 1) % TRAIL_LENGTH;
    p->trailPos[p->trailHead] = p->pos;
    p->trailAngle[p->trailHead] = p->rotation;

    // Propulsores: partículas cuando presiona W (detrás del jugador)
    if (IsKeyDown(KEY_W)) {
        Vector2 back = Vector2Add(p->pos, Vector2Scale(heading, -p->radius - 6.0f));
        for (int i=0;i<2;i++) SpawnWorldParticle(back, NEON_RED, 1.2f, 2.0f);
    }
}

void DrawPlayer(const Player* p) {
    // Ghost Trail (ring buffer)
    int idx = p->trailHead;
    for (int i=0;i<TRAIL_LENGTH;i++) {
        idx = (idx - 1 + TRAIL_LENGTH) % TRAIL_LENGTH;
        float a = 0.5f - i*0.04f;
        DrawCircleV(p->trailPos[idx], p->radius * (1.0f - i*0.02f), Fade(NEON_RED, a));
    }

    // Color shift: near-white/cyan during charge/execute
    Color col = NEON_RED;
    if (p->dashState == DASH_CHARGE) col = Fade(NEON_CYAN, 0.9f);
    else if (p->dashState == DASH_EXECUTE) col = WHITE;
    // Dibujar como un óvalo escalado (squash/stretch)
    // Usamos DrawPoly como triángulo estilizado de nave apuntando al mouse
    Vector2 center = p->pos;
    float rot = p->rotation;
    float sx = p->drawScale.x;
    float sy = p->drawScale.y;

    // Triángulo de nave
    Vector2 v1 = { center.x + cosf(rot*DEG2RAD) * p->radius * 1.2f * sx,
                   center.y + sinf(rot*DEG2RAD) * p->radius * 1.2f * sy };
    Vector2 v2 = { center.x + cosf((rot+140)*DEG2RAD) * p->radius * 0.9f * sx,
                   center.y + sinf((rot+140)*DEG2RAD) * p->radius * 0.9f * sy };
    Vector2 v3 = { center.x + cosf((rot-140)*DEG2RAD) * p->radius * 0.9f * sx,
                   center.y + sinf((rot-140)*DEG2RAD) * p->radius * 0.9f * sy };

    // Motion blur + chromatic aberration during dash (stronger and filled)
    bool dashing = (p->dashState == DASH_EXECUTE) || (p->dashState == DASH_RECOVERY);
    if (dashing) {
        float strength = (p->dashState == DASH_EXECUTE) ? 1.0f : 0.6f;
        float off = 6.0f * strength;
        Vector2 forward = (Vector2){ cosf(rot*DEG2RAD) * off, sinf(rot*DEG2RAD) * off };
        Vector2 back    = (Vector2){ -forward.x, -forward.y };
        // Cyan forward fill
        Vector2 v1c = Vector2Add(v1, forward);
        Vector2 v2c = Vector2Add(v2, forward);
        Vector2 v3c = Vector2Add(v3, forward);
        DrawTriangle(v1c, v2c, v3c, Fade(NEON_CYAN, 0.35f));
        DrawTriangleLines(v1c, v2c, v3c, Fade(NEON_CYAN, 0.75f));
        // Purple back fill
        Vector2 v1p = Vector2Add(v1, back);
        Vector2 v2p = Vector2Add(v2, back);
        Vector2 v3p = Vector2Add(v3, back);
        DrawTriangle(v1p, v2p, v3p, Fade(NEON_PURPLE, 0.30f));
        DrawTriangleLines(v1p, v2p, v3p, Fade(NEON_PURPLE, 0.70f));
        // White core glow during execute peak
        if (p->dashState == DASH_EXECUTE) {
            Vector2 v1w = { v1.x + forward.x*0.5f, v1.y + forward.y*0.5f };
            Vector2 v2w = { v2.x + forward.x*0.5f, v2.y + forward.y*0.5f };
            Vector2 v3w = { v3.x + forward.x*0.5f, v3.y + forward.y*0.5f };
            DrawTriangle(v1w, v2w, v3w, Fade(WHITE, 0.20f));
        }
    }
    // Invisible during the teleport frame (EXECUTE), keep only blur/glow
    if (p->dashState != DASH_EXECUTE) {
        // Neon red triangle with subtle glow (core)
        DrawTriangle(v1, v2, v3, Fade(col, 0.12f));
        DrawTriangleLines(v1, v2, v3, col);
    }
    Vector2 g1a = { v1.x + 1, v1.y + 1 }, g2a = { v2.x + 1, v2.y + 1 }, g3a = { v3.x + 1, v3.y + 1 };
    Vector2 g1b = { v1.x - 1, v1.y - 1 }, g2b = { v2.x - 1, v2.y - 1 }, g3b = { v3.x - 1, v3.y - 1 };
    if (p->dashState != DASH_EXECUTE) {
        DrawTriangleLines(g1a, g2a, g3a, Fade(col, 0.5f));
        DrawTriangleLines(g1b, g2b, g3b, Fade(col, 0.3f));
    }
}