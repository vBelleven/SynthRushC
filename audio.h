#ifndef AUDIO_H
#define AUDIO_H
#include "globals.h"

void InitAudioSystem();
void CloseAudioSystem();
void PlaySynthSound(const char* type); // "Shoot", "Explode", "Dash"

// UI SFX (Subtractive Synth style)
void PlayUIHover();     // Hover sobre botón
void PlayUIClick();     // Click en botón
void PlayTitleHover();  // Hover sobre el título "SynthRush"

// --- Síntesis en tiempo real (Mixer por voces) ---
typedef enum { WAVE_SINE, WAVE_SQUARE, WAVE_SAW, WAVE_NOISE } WaveType;

typedef struct {
	bool active;         // Slot activo
	WaveType type;       // Tipo de onda
	float freq;          // Frecuencia actual (Hz)
	float phase;         // 0.0 .. 1.0
	float volume;        // 0.0 .. 1.0
	float decayPerSample;// multiplicador por muestra (ej 0.999)
	float slidePerSample;// cambio de freq por muestra (Hz/muestra)
} SynthVoice;


#endif