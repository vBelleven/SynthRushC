#include "text.h"

static const char* TEXT_ES[TXT__COUNT] = {
    [TXT_PLAY] = "JUGAR",
    [TXT_MULTIPLAYER] = "MULTIJUGADOR",
    [TXT_OPTIONS] = "OPCIONES",
    [TXT_EXIT] = "SALIR",
    [TXT_FOOTER_CREATOR] = "Creado por Belleven, 2025",
    [TXT_FOOTER_VERSION] = "EARLY ACCESS V0.1",
};

static const char* TEXT_EN[TXT__COUNT] = {
    [TXT_PLAY] = "PLAY",
    [TXT_MULTIPLAYER] = "MULTIPLAYER",
    [TXT_OPTIONS] = "OPTIONS",
    [TXT_EXIT] = "EXIT",
    [TXT_FOOTER_CREATOR] = "Created by Belleven, 2025",
    [TXT_FOOTER_VERSION] = "EARLY ACCESS V0.1",
};

void InitText() {
    // No-op for now; reserved for future external loading
}

const char* GetText(TextId id) {
    if (id < 0 || id >= TXT__COUNT) return "";
    switch (currentLang) {
        case LANG_ES: return TEXT_ES[id];
        case LANG_EN: return TEXT_EN[id];
        default: return TEXT_EN[id];
    }
}
