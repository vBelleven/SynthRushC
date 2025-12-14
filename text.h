#ifndef TEXT_H
#define TEXT_H

#include "globals.h"

// Claves de texto públicas
typedef enum {
    TXT_PLAY,
    TXT_MULTIPLAYER,
    TXT_OPTIONS,
    TXT_EXIT,
    TXT_FOOTER_CREATOR,
    TXT_FOOTER_VERSION,
    TXT__COUNT
} TextId;

void InitText();
const char* GetText(TextId id);

#endif
