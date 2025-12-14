#include "config.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

Settings gameSettings;

void InitSettingsDefaults(Settings* s) {
    s->fullscreenMode = FS_WINDOWED;
    s->vsync = true;
    s->bloom = 0.6f;
    s->crt = false;
    s->resolutionW = SCREEN_WIDTH;
    s->resolutionH = SCREEN_HEIGHT;
    s->masterVolume = 0.9f;
    s->musicVolume = 0.6f;
    s->sfxVolume = 0.8f;
    s->muteOnFocusLoss = true;
    s->screenShake = 1.0f;
    s->damageNumbersMode = 1;
    s->language = LANG_ES;
}

static void ApplyImmediate(const Settings* s) {
    SetMasterVolume(s->masterVolume);
}

bool LoadSettings(const char* path, Settings* s) {
    FILE* f = fopen(path, "r");
    if (!f) return false;
    char key[64];
    char val[128];
    while (fscanf(f, "%63[^=]=%127s\n", key, val) == 2) {
        if (strcmp(key, "fullscreen") == 0) {
            if (strcmp(val, "windowed") == 0) s->fullscreenMode = FS_WINDOWED;
            else if (strcmp(val, "fullscreen") == 0) s->fullscreenMode = FS_FULLSCREEN;
            else if (strcmp(val, "borderless") == 0) s->fullscreenMode = FS_BORDERLESS;
        } else if (strcmp(key, "vsync") == 0) s->vsync = (strcmp(val, "1") == 0);
        else if (strcmp(key, "bloom") == 0) s->bloom = (float)atof(val);
        else if (strcmp(key, "crt") == 0) s->crt = (strcmp(val, "1") == 0);
        else if (strcmp(key, "resW") == 0) s->resolutionW = atoi(val);
        else if (strcmp(key, "resH") == 0) s->resolutionH = atoi(val);
        else if (strcmp(key, "master") == 0) s->masterVolume = (float)atof(val);
        else if (strcmp(key, "music") == 0) s->musicVolume = (float)atof(val);
        else if (strcmp(key, "sfx") == 0) s->sfxVolume = (float)atof(val);
        else if (strcmp(key, "muteLoss") == 0) s->muteOnFocusLoss = (strcmp(val, "1") == 0);
        else if (strcmp(key, "shake") == 0) s->screenShake = (float)atof(val);
        else if (strcmp(key, "damage") == 0) s->damageNumbersMode = atoi(val);
        else if (strcmp(key, "lang") == 0) s->language = (strcmp(val, "es") == 0) ? LANG_ES : LANG_EN;
    }
    fclose(f);
    ApplyImmediate(s);
    return true;
}

bool SaveSettings(const char* path, const Settings* s) {
    FILE* f = fopen(path, "w");
    if (!f) return false;
    fprintf(f, "fullscreen=%s\n", s->fullscreenMode==FS_WINDOWED?"windowed":s->fullscreenMode==FS_FULLSCREEN?"fullscreen":"borderless");
    fprintf(f, "vsync=%d\n", s->vsync?1:0);
    fprintf(f, "bloom=%.3f\n", s->bloom);
    fprintf(f, "crt=%d\n", s->crt?1:0);
    fprintf(f, "resW=%d\n", s->resolutionW);
    fprintf(f, "resH=%d\n", s->resolutionH);
    fprintf(f, "master=%.3f\n", s->masterVolume);
    fprintf(f, "music=%.3f\n", s->musicVolume);
    fprintf(f, "sfx=%.3f\n", s->sfxVolume);
    fprintf(f, "muteLoss=%d\n", s->muteOnFocusLoss?1:0);
    fprintf(f, "shake=%.3f\n", s->screenShake);
    fprintf(f, "damage=%d\n", s->damageNumbersMode);
    fprintf(f, "lang=%s\n", s->language==LANG_ES?"es":"en");
    fclose(f);
    return true;
}
