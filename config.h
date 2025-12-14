#ifndef CONFIG_H
#define CONFIG_H

#include "globals.h"

void InitSettingsDefaults(Settings* s);
bool LoadSettings(const char* path, Settings* s);
bool SaveSettings(const char* path, const Settings* s);

#endif
