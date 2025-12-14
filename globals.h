#ifndef GLOBALS_H
#define GLOBALS_H

#include "raylib.h"

// --- CONSTANTES ---
#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600
#define MAX_PARTICLES 1000

// --- COLORES NEON ---
// Definimos los colores aquí para usarlos en UI, Player y Enemigos
static const Color NEON_RED = {255, 20, 80, 255};
static const Color NEON_CYAN = {0, 240, 255, 255};
static const Color NEON_PURPLE = {180, 20, 255, 255};
static const Color NEON_WHITE = {220, 230, 255, 255};
static const Color DARK_BG = {10, 10, 15, 255};

// --- ESTADOS DEL JUEGO ---
typedef enum {
    STATE_MENU,
    STATE_GAMEPLAY,
    STATE_MULTIPLAYER_LOBBY,
    STATE_OPTIONS,
    STATE_EXIT
} GameState;

// --- IDIOMA ---
typedef enum { LANG_ES, LANG_EN } Language;
extern Language currentLang; // "extern" dice que la variable vive en otro lado (main.c)

// --- OBJETOS ---
typedef enum { DASH_IDLE, DASH_CHARGE, DASH_EXECUTE, DASH_RECOVERY } DashState;

typedef struct {
    Vector2 pos;
    Vector2 vel;
    Vector2 acc;
    float rotation;
    float radius;
    Color color;
    // Dash
    DashState dashState;
    float dashTimer;
    float dashCooldown;
    // Visuals
    Vector2 drawScale;
    // Ghost trail ring buffer
    #define TRAIL_LENGTH 10
    Vector2 trailPos[TRAIL_LENGTH];
    float trailAngle[TRAIL_LENGTH];
    int trailHead;
} Player;

// --- SETTINGS ---
typedef enum { FS_WINDOWED, FS_FULLSCREEN, FS_BORDERLESS } FullscreenMode;

typedef struct {
    // Video
    FullscreenMode fullscreenMode;
    bool vsync;
    float bloom;   // 0..1
    bool crt;
    int resolutionW;
    int resolutionH;
    // Audio
    float masterVolume; // 0..1
    float musicVolume;  // 0..1
    float sfxVolume;    // 0..1
    bool muteOnFocusLoss;
    // Gameplay
    float screenShake; // 0..2 (200%)
    int damageNumbersMode; // 0=off,1=on,2=crit-only
    Language language; // mirrors currentLang
} Settings;

extern Settings gameSettings;

// --- DASH FX GLOBALS ---
extern float gHitstopTimer;      // >0 freezes updates for 1 frame
extern Vector2 gDashA;           // origin of dash
extern Vector2 gDashB;           // destination of dash
extern float gShockRingTimer;    // ring expand timer
extern float gScreenShake;       // shake intensity impulse
extern float gGhostTimer;        // draw A->B ghosts for a short time
extern int   gChargeActive;      // suction during charge
extern Vector2 gChargeCenter;    // player pos during charge

#endif