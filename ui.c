#include "ui.h"
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include "audio.h"
#include "config.h"
#include "text.h"
#include "text.h"

// --- TEXTOS LOCALIZADOS --- (migrados a text.c)

// --- ESTRUCTURA INTERNA DE PARTÍCULAS ---
typedef struct {
    Vector2 pos;
    Vector2 vel;
    float life;
    float size;
    Color color;      // Color inicial
    Color colorEnd;   // Color final al desvanecerse
    float decay;
} UIParticle;

#define MAX_UI_PARTICLES 300
static UIParticle particles[MAX_UI_PARTICLES];
static UIParticle worldParticles[MAX_UI_PARTICLES];
static bool uiHoverStates[6] = { false, false, false, false, false, false };
static double uiHoverLastPlay[6] = { 0 };

// --- FUNCIONES DEL SISTEMA DE PARTÍCULAS ---

void InitUI() {
    for (int i = 0; i < MAX_UI_PARTICLES; i++) particles[i].life = 0;
    for (int i = 0; i < MAX_UI_PARTICLES; i++) worldParticles[i].life = 0;
}

void SpawnParticle(Vector2 pos, Color color, float speedMult, float sizeBase) {
    for (int i = 0; i < MAX_UI_PARTICLES; i++) {
        if (particles[i].life <= 0) {
            particles[i].life = 1.0f;
            particles[i].pos = pos;
            particles[i].color = color;
            // Transición de color: rojo -> cian por defecto
            particles[i].colorEnd = NEON_CYAN;
            particles[i].size = (float)GetRandomValue(2, (int)(sizeBase * 10)) / 10.0f;
            particles[i].decay = (float)GetRandomValue(1, 5) / 120.0f; // Vida ligeramente más larga
            float angle = (float)GetRandomValue(0, 360) * DEG2RAD;
            float speed = (float)GetRandomValue(50, 200) / 100.0f * speedMult;
            particles[i].vel = (Vector2){ cosf(angle) * speed, sinf(angle) * speed };
            break; 
        }
    }
}

void SpawnExplosion(Vector2 pos, Color color) {
    for(int i=0; i<15; i++) SpawnParticle(pos, color, 2.5f, 5.0f);
}

void SpawnWorldParticle(Vector2 pos, Color color, float speedMult, float sizeBase) {
    for (int i = 0; i < MAX_UI_PARTICLES; i++) {
        if (worldParticles[i].life <= 0) {
            worldParticles[i].life = 1.0f;
            worldParticles[i].pos = pos;
            worldParticles[i].color = color;
            worldParticles[i].colorEnd = NEON_CYAN;
            worldParticles[i].size = (float)GetRandomValue(2, (int)(sizeBase * 10)) / 10.0f;
            worldParticles[i].decay = (float)GetRandomValue(1, 5) / 120.0f;
            float angle = (float)GetRandomValue(0, 360) * DEG2RAD;
            float speed = (float)GetRandomValue(50, 200) / 100.0f * speedMult;
            worldParticles[i].vel = (Vector2){ cosf(angle) * speed, sinf(angle) * speed };
            break;
        }
    }
}

void SpawnWorldExplosion(Vector2 pos, Color color) {
    for(int i=0; i<15; i++) SpawnWorldParticle(pos, color, 2.5f, 5.0f);
}

void UpdateParticles() {
    for (int i = 0; i < MAX_UI_PARTICLES; i++) {
        if (particles[i].life > 0) {
            particles[i].pos.x += particles[i].vel.x;
            particles[i].pos.y += particles[i].vel.y;
            particles[i].life -= particles[i].decay;
            particles[i].size *= 0.95f;
            particles[i].vel.x *= 0.98f;
            particles[i].vel.y *= 0.98f;
        }
    }
}

void UpdateWorldParticles() {
    for (int i = 0; i < MAX_UI_PARTICLES; i++) {
        if (worldParticles[i].life > 0) {
            worldParticles[i].pos.x += worldParticles[i].vel.x;
            worldParticles[i].pos.y += worldParticles[i].vel.y;
            worldParticles[i].life -= worldParticles[i].decay;
            worldParticles[i].size *= 0.95f;
            worldParticles[i].vel.x *= 0.98f;
            worldParticles[i].vel.y *= 0.98f;
            // Succión de luz durante la carga del dash
            extern int gChargeActive; extern Vector2 gChargeCenter;
            if (gChargeActive) {
                Vector2 toCenter = { gChargeCenter.x - worldParticles[i].pos.x, gChargeCenter.y - worldParticles[i].pos.y };
                float d = sqrtf(toCenter.x*toCenter.x + toCenter.y*toCenter.y);
                if (d > 0.001f) {
                    toCenter.x /= d; toCenter.y /= d;
                    worldParticles[i].vel.x += toCenter.x * 1.2f; // succión más fuerte
                    worldParticles[i].vel.y += toCenter.y * 1.2f;
                }
            }
            // Impulso sutil del polvo cercano cuando ocurre el shock ring del dash
            extern float gShockRingTimer; extern Vector2 gDashA;
            if (gShockRingTimer > 0.0f) {
                // Radio dinámico del anillo (crece y desvanece)
                float t = gShockRingTimer; // asume rango 0..1
                float ringR = 30.0f + 140.0f * t;
                Vector2 fromOrigin = { worldParticles[i].pos.x - gDashA.x, worldParticles[i].pos.y - gDashA.y };
                float d2 = fromOrigin.x*fromOrigin.x + fromOrigin.y*fromOrigin.y;
                float d = sqrtf(d2);
                if (d < ringR) {
                    if (d > 0.0001f) { fromOrigin.x /= d; fromOrigin.y /= d; }
                    // Empuje hacia afuera con caída suave
                    float falloff = 1.0f - (d / ringR);
                    worldParticles[i].vel.x += fromOrigin.x * (0.8f * falloff);
                    worldParticles[i].vel.y += fromOrigin.y * (0.8f * falloff);
                }
            }
        }
    }
}

static Color LerpColor(Color a, Color b, float t) {
    if (t < 0) t = 0; if (t > 1) t = 1;
    Color c;
    c.r = (unsigned char)((1.0f - t) * a.r + t * b.r);
    c.g = (unsigned char)((1.0f - t) * a.g + t * b.g);
    c.b = (unsigned char)((1.0f - t) * a.b + t * b.b);
    c.a = (unsigned char)255;
    return c;
}

void DrawParticles() {
    for (int i = 0; i < MAX_UI_PARTICLES; i++) {
        if (particles[i].life > 0) {
            float t = 1.0f - particles[i].life; // 0 -> 1 mientras muere
            Color c = LerpColor(particles[i].color, particles[i].colorEnd, t);
            DrawCircleV(particles[i].pos, particles[i].size, Fade(c, particles[i].life));
        }
    }
}

void DrawWorldParticles() {
    for (int i = 0; i < MAX_UI_PARTICLES; i++) {
        if (worldParticles[i].life > 0) {
            float t = 1.0f - worldParticles[i].life;
            Color c = LerpColor(worldParticles[i].color, worldParticles[i].colorEnd, t);
            DrawCircleV(worldParticles[i].pos, worldParticles[i].size, Fade(c, worldParticles[i].life));
        }
    }
}

// --- FUNCIONES VISUALES DEL MENÚ ---

void DrawSynthwaveGrid() {
    float time = (float)GetTime();
    int scrW = GetScreenWidth();
    int scrH = GetScreenHeight();
    
    // El horizonte siempre está al 50% de la pantalla (funciona en 600px y en 1080px)
    float horizon = scrH * 0.5f; 
    
    // Fondo negro abajo para limpiar frames anteriores
    DrawRectangle(0, (int)horizon, scrW, scrH - (int)horizon, DARK_BG);

    // Líneas Horizontales (Suelo)
    // Movimiento continuo infinito: desplazamiento lineal sin reinicios bruscos
    float spacing = 40.0f;
    float baseShift = time * 100.0f; // velocidad del suelo
    // Cubrimos un rango amplio de líneas para que, al desplazarse, siempre haya líneas visibles
    int totalLines = (int)((scrH - horizon) / spacing) + 10;
    for (int i = -totalLines; i <= totalLines * 2; i++) {
        float y_pos = horizon + i * spacing + fmodf(baseShift, spacing);
        if (y_pos >= horizon && y_pos <= scrH) {
            float alpha = (y_pos - horizon) / (scrH - horizon);
            DrawLine(0, (int)y_pos, scrW, (int)y_pos, Fade(NEON_PURPLE, alpha * 0.5f));
        }
    }

    // Líneas Verticales (Perspectiva)
    float centerX = scrW / 2.0f;
    // Dibujamos suficientes líneas para llenar pantallas anchas (Widescreen)
    int numLines = (scrW / 40) + 2; 
    
    for (int i = -numLines; i <= numLines; i++) {
        float x_start = centerX + (i * 100); // Separación base
        
        // Punto final (abajo del todo)
        // El factor (i * 300) hace que se abran en abanico
        float x_end = centerX + (i * 400) + (sinf(time * 0.5f) * 50.0f);
        
        DrawLine((int)centerX, (int)horizon, (int)x_end, scrH, Fade(NEON_PURPLE, 0.3f));
    }
    
    // Sol de Neón (Siempre centrado y sobre el horizonte)
    DrawCircle((int)centerX, (int)horizon - 40, scrH * 0.15f, Fade(NEON_RED, 0.8f));
    // Tapar la parte inferior del sol con el "suelo"
    DrawRectangle(0, (int)horizon, scrW, scrH, Fade(DARK_BG, 0.6f));

    // CRT Scanlines overlay (simple): líneas horizontales semitransparentes
    if (gameSettings.crt) {
        for (int y = 0; y < scrH; y += 2) {
            DrawLine(0, y, scrW, y, Fade(BLACK, 0.12f));
        }
    }
}

// --- COMPONENTES UI ---

bool DrawJuicyButton(Rectangle rect, int textIndex, int fontSize) {
    Vector2 mouse = GetMousePosition();
    bool isHover = CheckCollisionPointRec(mouse, rect);
    
    float time = (float)GetTime();
    float wave = isHover ? sinf(time * 18.0f) : sinf(time * 3.0f);
    float scale = isHover ? 1.08f + (wave * 0.02f) : 1.0f;

    // Centrar escalado
    float w = rect.width * scale;
    float h = rect.height * scale;
    float x = rect.x - (w - rect.width) / 2;
    float y = rect.y - (h - rect.height) / 2;

    // Eliminamos el vaivén lateral/vertical; preferimos un leve "rotar" del texto
    Rectangle drawRect = { x, y, w, h };

    Color borderCol = isHover ? NEON_CYAN : Fade(WHITE, 0.5f);
    Color textCol = isHover ? NEON_CYAN : WHITE;
    float bloom = gameSettings.bloom; // 0..1
    Color glowCol = isHover ? Fade(NEON_CYAN, 0.15f + 0.35f * bloom) : Fade(BLACK, 0.0f);

    // Glow suave similar al anterior
    DrawRectangleRec((Rectangle){x-5, y-5, w+10, h+10}, glowCol);
    DrawRectangleLinesEx(drawRect, isHover ? 2.0f : 1.0f, borderCol);

    const char* txt = (textIndex == 0) ? GetText(TXT_PLAY)
                        : (textIndex == 1) ? GetText(TXT_MULTIPLAYER)
                        : (textIndex == 2) ? GetText(TXT_OPTIONS)
                        : (textIndex == 3) ? GetText(TXT_EXIT)
                        : "";
    int txtW = MeasureText(txt, (int)(fontSize * scale));
    // Levísimo giro del texto cuando hover
    float angle = isHover ? sinf(time * 4.0f) * 3.0f : 0.0f; // grados
    Font df = GetFontDefault();
    Vector2 pos = { x + w/2 - txtW/2, y + h/2 - (fontSize * scale)/2 };
    Vector2 origin = { (float)txtW/2, (float)(fontSize * scale)/2 };
    pos.x += origin.x; pos.y += origin.y; // Ajustar porque origin rota en el centro
    DrawTextPro(df, txt, pos, origin, angle, (int)(fontSize * scale), 2.0f, textCol);

    if (isHover) {
        // Preciso, no repetitivo: sonar solo al entrar en hover
        if (!uiHoverStates[textIndex]) {
            PlayUIHover();
            uiHoverStates[textIndex] = true;
            uiHoverLastPlay[textIndex] = GetTime();
        }
        // Emitir pequeñas chispas en los bordes con ritmo sinusoidal
        int sparks = 2;
        for (int s = 0; s < sparks; s++) {
            if (GetRandomValue(0, 100) < 35) {
                float edgeX = x + GetRandomValue(0, (int)w);
                float edgeY = (GetRandomValue(0,1) ? y : (y + h));
                Vector2 edgePos = { edgeX, edgeY };
                SpawnParticle(edgePos, NEON_CYAN, 1.4f, 2.2f);
            }
        }
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            PlayUIClick();
            SpawnExplosion((Vector2){ x + w/2, y + h/2 }, NEON_CYAN);
            return true;
        }
    }
    else {
        uiHoverStates[textIndex] = false;
    }
    return false;
}

void DrawMainMenu(GameState* currentState) {
    DrawSynthwaveGrid();

    int scrW = GetScreenWidth();
    int scrH = GetScreenHeight();

    // 1. Título "SynthRush" (Posición relativa: 15% desde arriba)
    int titleFontSize = (scrH < 700) ? 60 : 100; // Fuente más pequeña si la ventana es chica
    const char* titleText = "SynthRush";
    int titleW = MeasureText(titleText, titleFontSize);
    int titleX = scrW/2 - titleW/2;
    int titleY = (int)(scrH * 0.15f);
    
    float time = (float)GetTime();
    float shakeX = sinf(time * 10.0f) * 3.0f;
    float shakeY = cosf(time * 8.0f) * 2.0f;

    // Interacción con mouse sobre el NOMBRE (no el mouse emitiendo)
    Rectangle titleRect = { (float)titleX, (float)titleY, (float)titleW, (float)titleFontSize };
    Vector2 mouse = GetMousePosition();
    bool overTitle = CheckCollisionPointRec(mouse, titleRect);
    static bool wasOverTitle = false;
    static double titleLastPlay = 0.0;
    if (overTitle) {
        double nowT = GetTime();
        // Neon repetitive while hovering the logo: fire every ~0.12s
        if (!wasOverTitle || (nowT - titleLastPlay) > 0.12) {
            PlayTitleHover();
            titleLastPlay = nowT;
        }
    }
    wasOverTitle = overTitle;

    // Dinámica de color: gradiente que cambia según la posición del mouse dentro del título
    float xRatio = overTitle ? (mouse.x - titleRect.x) / (titleRect.width > 0 ? titleRect.width : 1) : 0.5f;
    if (xRatio < 0) xRatio = 0; if (xRatio > 1) xRatio = 1;
    Color leftGlow  = LerpColor(NEON_RED, NEON_PURPLE, xRatio);
    Color rightGlow = LerpColor(NEON_CYAN, WHITE, xRatio);

    // Modulación de "shake" para hacerlo más hipnótico al pasar el mouse
    float shakeMult = overTitle ? 1.8f : 1.0f;
    float sx = shakeMult * shakeX;
    float sy = shakeMult * shakeY;

    DrawText(titleText, titleX + 4 + (int)sx, titleY + 4 + (int)sy, titleFontSize, Fade(leftGlow, 0.6f));
    DrawText(titleText, titleX - 2, titleY, titleFontSize, Fade(rightGlow, 0.6f));
    DrawText(titleText, titleX, titleY, titleFontSize, WHITE);

    // Emisión base desde el NOMBRE: partículas que nacen en el área del título
    for (int i = 0; i < (overTitle ? 6 : 3); i++) {
        if (GetRandomValue(0, 100) < (overTitle ? 75 : 35)) {
            Vector2 pos = {
                (float)titleX + GetRandomValue(0, titleW),
                (float)titleY + GetRandomValue(0, titleFontSize)
            };
            Color startCol = LerpColor(NEON_RED, NEON_PURPLE, xRatio);
            Color endCol   = LerpColor(NEON_CYAN, WHITE, xRatio);
            SpawnParticle(pos, startCol, overTitle ? 2.0f : 1.4f, overTitle ? 4.5f : 4.0f);
            // Ajustar el color final en las partículas recién creadas cerca del punto
            for (int p = 0; p < MAX_UI_PARTICLES; p++) {
                if (particles[p].life > 0 && fabsf(particles[p].pos.x - pos.x) < 2.0f && fabsf(particles[p].pos.y - pos.y) < 2.0f) {
                    particles[p].colorEnd = endCol;
                    if (overTitle) {
                        particles[p].decay *= 0.85f;
                        particles[p].size *= 1.1f;
                    }
                    break;
                }
            }
        }
    }

    // 2. Botones (Posición relativa: empiezan al 40% de la pantalla)
    // Reducimos el espacio entre botones (gap) si la pantalla es pequeña
    float startY = scrH * 0.40f; 
    float gap = (scrH < 700) ? 60.0f : 80.0f; 
    float btnW = 300;
    float btnH = (scrH < 700) ? 40.0f : 50.0f; // Botones más finos en baja res
    float btnX = scrW/2 - btnW/2;
    int fontSize = (scrH < 700) ? 20 : 30;

    if (DrawJuicyButton((Rectangle){btnX, startY, btnW, btnH}, 0, fontSize)) *currentState = STATE_GAMEPLAY;
    if (DrawJuicyButton((Rectangle){btnX, startY + gap, btnW, btnH}, 1, fontSize)) *currentState = STATE_MULTIPLAYER_LOBBY;
    if (DrawJuicyButton((Rectangle){btnX, startY + gap*2, btnW, btnH}, 2, fontSize)) {
        // Entrar al menú de Opciones
        *currentState = STATE_OPTIONS;
        SpawnExplosion((Vector2){btnX + btnW/2, startY + gap*2 + 25}, NEON_PURPLE);
    }
    if (DrawJuicyButton((Rectangle){btnX, startY + gap*3, btnW, btnH}, 3, fontSize)) *currentState = STATE_EXIT;

    // 3. Footer (Alineado a la derecha abajo, SIEMPRE visible)
    const char* txtCreator = GetText(TXT_FOOTER_CREATOR);
    const char* txtVersion = GetText(TXT_FOOTER_VERSION);
    
    int wCreator = MeasureText(txtCreator, 20);
    int wVersion = MeasureText(txtVersion, 20);
    
    // Margen de 20px desde la derecha y 30px desde abajo
    DrawText(txtCreator, scrW - wCreator - 20, scrH - 30, 20, GRAY);
    DrawText(txtVersion, scrW - wVersion - 20, scrH - 55, 20, NEON_CYAN);
}

// --- OPTIONS MENU ---
static float DrawSlider(Rectangle rect, float value01) {
    Vector2 m = GetMousePosition();
    bool hover = CheckCollisionPointRec(m, rect);
    float t = value01;
    if (hover && IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
        float rel = (m.x - rect.x) / rect.width; if (rel < 0) rel = 0; if (rel > 1) rel = 1;
        t = rel;
    }
    DrawRectangleRec(rect, Fade(WHITE, 0.1f));
    DrawRectangle(rect.x, rect.y, rect.width * t, rect.height, Fade(NEON_CYAN, 0.5f));
    DrawRectangleLinesEx(rect, 2.0f, NEON_CYAN);
    return t;
}

static bool DrawToggle(Rectangle rect, bool on, const char* label) {
    DrawRectangleRec(rect, Fade(WHITE, 0.1f));
    DrawRectangleLinesEx(rect, 2.0f, on ? NEON_CYAN : Fade(WHITE, 0.4f));
    DrawText(label, rect.x + 10, rect.y + 6, 20, on ? NEON_CYAN : WHITE);
    Vector2 m = GetMousePosition();
    if (CheckCollisionPointRec(m, rect) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) return !on;
    return on;
}

void DrawOptionsMenu(GameState* currentState) {
    int scrW = GetScreenWidth();
    int scrH = GetScreenHeight();
    int fontTitle = (scrH < 650) ? 32 : ((scrH < 800) ? 40 : 60);
    DrawText("OPCIONES", scrW/2 - MeasureText("OPCIONES", fontTitle)/2, (int)(scrH*0.10f), fontTitle, WHITE);

    // Tabs (responsive)
    float tabW = (scrW < 900 ? 120.0f : 190.0f);
    float tabH = (scrH < 650 ? 28.0f : 40.0f);
    float x = scrW*0.08f; float y = scrH*0.18f;
    Rectangle rVideo = (Rectangle){x, y, tabW, tabH};
    Rectangle rAudio = (Rectangle){x+tabW+8, y, tabW, tabH};
    Rectangle rGameplay = (Rectangle){x+2*(tabW+8), y, tabW, tabH};
    Rectangle rControls = (Rectangle){x+3*(tabW+8), y, tabW, tabH};
    static int tab = 0; // 0=video,1=audio,2=gameplay,3=controls
    if (DrawToggle(rVideo, tab==0, "VIDEO") != (tab==0)) tab = 0;
    if (DrawToggle(rAudio, tab==1, "AUDIO") != (tab==1)) tab = 1;
    if (DrawToggle(rGameplay, tab==2, "GAMEPLAY") != (tab==2)) tab = 2;
    if (DrawToggle(rControls, tab==3, "CONTROLES") != (tab==3)) tab = 3;

    float panelX = x; float panelY = y + tabH + 16; float panelW = scrW*0.84f; float panelH = scrH*0.60f;
    if (panelX + panelW > scrW) panelW = scrW - panelX - 10;
    DrawRectangleLines(panelX, panelY, panelW, panelH, NEON_PURPLE);

    float rowY = panelY + 16; float rowH = (scrH < 650 ? 22.0f : 28.0f); float colX = panelX + 16; float colW = panelW - 32;

    if (tab == 0) {
        // VIDEO
        DrawText("Modo Pantalla", colX, rowY, (scrH<650?18:22), WHITE);
        Rectangle rMode = (Rectangle){colX + 180, rowY-4, 200, rowH};
        bool isWin = gameSettings.fullscreenMode == FS_WINDOWED;
        bool isFull = gameSettings.fullscreenMode == FS_FULLSCREEN;
        bool isBorder = gameSettings.fullscreenMode == FS_BORDERLESS;
        if (DrawToggle(rMode, isWin, "Ventana") != isWin) {
            gameSettings.fullscreenMode = FS_WINDOWED;
            int monitor = GetCurrentMonitor();
            if (IsWindowFullscreen()) ToggleFullscreen();
            SetWindowSize(SCREEN_WIDTH, SCREEN_HEIGHT);
            SetWindowPosition(GetMonitorWidth(monitor)/2 - SCREEN_WIDTH/2,
                              GetMonitorHeight(monitor)/2 - SCREEN_HEIGHT/2);
        }
        Rectangle rMode2 = (Rectangle){rMode.x + tabW, rMode.y, rMode.width, rMode.height};
        if (DrawToggle(rMode2, isFull, "Pantalla Completa") != isFull) {
            gameSettings.fullscreenMode = FS_FULLSCREEN;
            int monitor = GetCurrentMonitor();
            int monitorW = GetMonitorWidth(monitor);
            int monitorH = GetMonitorHeight(monitor);
            SetWindowSize(monitorW, monitorH);
            ToggleFullscreen();
        }
        Rectangle rMode3 = (Rectangle){rMode2.x + tabW, rMode.y, rMode.width, rMode.height};
        if (DrawToggle(rMode3, isBorder, "Sin Bordes") != isBorder) {
            gameSettings.fullscreenMode = FS_BORDERLESS;
            int monitor = GetCurrentMonitor();
            int monitorW = GetMonitorWidth(monitor);
            int monitorH = GetMonitorHeight(monitor);
            ClearWindowState(FLAG_WINDOW_UNDECORATED);
            SetWindowState(FLAG_WINDOW_UNDECORATED);
            SetWindowSize(monitorW, monitorH);
        }
        rowY += (scrH<650?40:50);
        DrawText("VSync", colX, rowY, 22, WHITE);
        Rectangle rV = (Rectangle){colX + 180, rowY-4, 100, rowH};
        bool vs = DrawToggle(rV, gameSettings.vsync, gameSettings.vsync?"ON":"OFF");
        if (vs != gameSettings.vsync) { gameSettings.vsync = vs; SetConfigFlags(gameSettings.vsync?FLAG_VSYNC_HINT:0); }

        rowY += (scrH<650?40:50);
        DrawText("Bloom", colX, rowY, 22, WHITE);
        Rectangle rB = (Rectangle){colX + 180, rowY-4, (scrW<900?220:300), rowH};
        gameSettings.bloom = DrawSlider(rB, gameSettings.bloom);

        rowY += (scrH<650?40:50);
        DrawText("Filtro CRT", colX, rowY, 22, WHITE);
        Rectangle rC = (Rectangle){colX + 180, rowY-4, 140, rowH};
        gameSettings.crt = DrawToggle(rC, gameSettings.crt, gameSettings.crt?"ON":"OFF");
    }
    else if (tab == 1) {
        // AUDIO
        DrawText("Master", colX, rowY, 22, WHITE);
        Rectangle rM = (Rectangle){colX + 180, rowY-4, (scrW<900?220:300), rowH};
        gameSettings.masterVolume = DrawSlider(rM, gameSettings.masterVolume);
        SetMasterVolume(gameSettings.masterVolume);

        rowY += (scrH<650?40:50);
        DrawText("Música", colX, rowY, 22, WHITE);
        Rectangle rMu = (Rectangle){colX + 180, rowY-4, (scrW<900?220:300), rowH};
        gameSettings.musicVolume = DrawSlider(rMu, gameSettings.musicVolume);

        rowY += (scrH<650?40:50);
        DrawText("SFX", colX, rowY, 22, WHITE);
        Rectangle rS = (Rectangle){colX + 180, rowY-4, (scrW<900?220:300), rowH};
        float prevSfx = gameSettings.sfxVolume;
        gameSettings.sfxVolume = DrawSlider(rS, gameSettings.sfxVolume);
        if (fabsf(gameSettings.sfxVolume - prevSfx) > 0.02f) PlayUIHover();

        rowY += (scrH<650?40:50);
        DrawText("Mute al perder foco", colX, rowY, 22, WHITE);
        Rectangle rMF = (Rectangle){colX + 180, rowY-4, 140, rowH};
        gameSettings.muteOnFocusLoss = DrawToggle(rMF, gameSettings.muteOnFocusLoss, gameSettings.muteOnFocusLoss?"ON":"OFF");
    }
    else if (tab == 2) {
        // GAMEPLAY
        DrawText("Screen Shake", colX, rowY, 22, WHITE);
        Rectangle rSS = (Rectangle){colX + 180, rowY-4, (scrW<900?220:300), rowH};
        gameSettings.screenShake = DrawSlider(rSS, gameSettings.screenShake);

        rowY += (scrH<650?40:50);
        DrawText("Números de daño", colX, rowY, 22, WHITE);
        Rectangle rDN0 = (Rectangle){colX + 180, rowY-4, 100, rowH};
        Rectangle rDN1 = (Rectangle){colX + 284, rowY-4, 100, rowH};
        Rectangle rDN2 = (Rectangle){colX + 388, rowY-4, 140, rowH};
        if (DrawToggle(rDN0, gameSettings.damageNumbersMode==0, "OFF") != (gameSettings.damageNumbersMode==0)) gameSettings.damageNumbersMode=0;
        if (DrawToggle(rDN1, gameSettings.damageNumbersMode==1, "ON") != (gameSettings.damageNumbersMode==1)) gameSettings.damageNumbersMode=1;
        if (DrawToggle(rDN2, gameSettings.damageNumbersMode==2, "Solo Crit") != (gameSettings.damageNumbersMode==2)) gameSettings.damageNumbersMode=2;

        rowY += (scrH<650?40:50);
        DrawText("Idioma", colX, rowY, 22, WHITE);
        Rectangle rLE = (Rectangle){colX + 180, rowY-4, 130, rowH};
        Rectangle rLI = (Rectangle){colX + 320, rowY-4, 130, rowH};
        bool es = DrawToggle(rLE, gameSettings.language==LANG_ES, "Español");
        bool en = DrawToggle(rLI, gameSettings.language==LANG_EN, "English");
        if (es) { gameSettings.language = LANG_ES; currentLang = LANG_ES; }
        else if (en) { gameSettings.language = LANG_EN; currentLang = LANG_EN; }
    }
    else if (tab == 3) {
        // CONTROLES
        int f = (scrH<650?18:22);
        DrawText("Arriba: W", colX, rowY, f, WHITE);
        rowY += (scrH<650?28:35); DrawText("Abajo: S", colX, rowY, f, WHITE);
        rowY += (scrH<650?28:35); DrawText("Dash: E / Espacio / Click Derecho", colX, rowY, f, WHITE);
    }

    // Back button
    float backY = panelY + panelH + (scrH<650?10:20);
    if (DrawJuicyButton((Rectangle){scrW/2 - (scrW<900?90:120), backY, (scrW<900?180:240), (scrH<650?32:40)}, 3, (scrH<650?20:24))) {
        SaveSettings("synthrush.cfg", &gameSettings);
        *currentState = STATE_MENU;
    }
}