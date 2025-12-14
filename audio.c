#include "audio.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

// --- UI pre-baked SFX (optional, kept for clicks) ---
static Sound sfxHover;
static Sound sfxClick;
// Title hover needs overlap; use a small pool for polyphony
#define TITLE_POOL_SIZE 4
static Sound sfxTitleHoverPool[TITLE_POOL_SIZE];
static int sfxTitleHoverIndex = 0;

// --- Real-time synth mixer ---
#define SYNTH_SAMPLE_RATE 44100
#define SYNTH_CHANNELS 1
#define SYNTH_BUFFER_SAMPLES 1024
#define MAX_VOICES 32

static SynthVoice voices[MAX_VOICES];
static AudioStream synthStream;

static float randf_range(float a, float b) {
    return a + (b - a) * ((float)GetRandomValue(0, 10000) / 10000.0f);
}

static inline float voiceSample(SynthVoice* v) {
    float s = 0.0f;
    switch (v->type) {
        case WAVE_SINE:  s = sinf(2.0f * PI * v->phase); break;
        case WAVE_SQUARE: s = (v->phase < 0.5f) ? 1.0f : -1.0f; break;
        case WAVE_SAW:   s = (2.0f * v->phase - 1.0f); break;
        case WAVE_NOISE: s = randf_range(-1.0f, 1.0f); break;
        default: break;
    }
    return s * v->volume;
}

static void updateVoice(SynthVoice* v) {
    // phase accumulator
    v->phase += v->freq / (float)SYNTH_SAMPLE_RATE;
    if (v->phase >= 1.0f) v->phase -= 1.0f;
    // decay & slide
    v->volume *= v->decayPerSample;
    v->freq += v->slidePerSample;
    if (v->volume < 0.0005f) v->active = false;
    if (v->freq < 10.0f) v->freq = 10.0f;
}

// Raylib audio stream callback signature: void (*AudioCallback)(void *bufferData, unsigned int frames)
static void SynthCallback(void* bufferData, unsigned int frames) {
    float* out = (float*)bufferData;
    for (unsigned int i = 0; i < frames; i++) {
        float mix = 0.0f;
        for (int v = 0; v < MAX_VOICES; v++) {
            if (!voices[v].active) continue;
            mix += voiceSample(&voices[v]);
            updateVoice(&voices[v]);
        }
        // simple soft clip
        if (mix > 1.0f) mix = 1.0f; else if (mix < -1.0f) mix = -1.0f;
        out[i] = mix;
    }
}

void InitAudioSystem() {
    InitAudioDevice();
    SetMasterVolume(1.0f);
    // Pre-generate minimal UI SFX using raylib's beep as fallback (optional)
    // Keep click as a short sine burst via generated wave
    {
        // Brighter, precise hover: short square+sine blend with snap attack
        int sr = SYNTH_SAMPLE_RATE; int samples = (int)(0.06f * sr);
        short* data = (short*)MemAlloc(samples * sizeof(short));
        float baseFreq = 760.0f;
        for (int i=0;i<samples;i++){
            float t=i/(float)sr;
            float env=(t<0.004f)?(t/0.004f):fmaxf(0.0f, 1.0f-(t-0.004f)/0.056f);
            float phase=fmodf(t*baseFreq,1.0f);
            float square = (phase<0.5f)?1.0f:-1.0f;
            float sine = sinf(2*PI*baseFreq*t);
            float s=(0.65f*square+0.35f*sine)*0.40f*env;
            data[i]=(short)(s*32767);
        }
        Wave w = (Wave){ .data=data, .frameCount=samples, .sampleRate=sr, .sampleSize=16, .channels=1};
        sfxHover = LoadSoundFromWave(w);
        UnloadWave(w);
    }
    {
        int sr = SYNTH_SAMPLE_RATE; int samples = (int)(0.10f * sr);
        short* data = (short*)MemAlloc(samples * sizeof(short));
        for (int i=0;i<samples;i++){ float t=i/(float)sr; float env=(t<0.005f)?t/0.005f:(1.0f-(t-0.005f)/0.095f); float s=sinf(2*PI*220*t)*0.40f*env; data[i]=(short)(s*32767);} 
        Wave w = (Wave){ .data=data, .frameCount=samples, .sampleRate=sr, .sampleSize=16, .channels=1};
        sfxClick = LoadSoundFromWave(w);
        UnloadWave(w);
    }
    {
        // Neon cyberpunk loop: short bright saw+square at higher freq
        int sr = SYNTH_SAMPLE_RATE; int samples = (int)(0.085f * sr);
        short* data = (short*)MemAlloc(samples * sizeof(short));
        float baseFreq = 900.0f;
        for (int i=0;i<samples;i++){
            float t=i/(float)sr;
            float env=(t<0.003f)?(t/0.003f):fmaxf(0.0f, 1.0f-(t-0.003f)/0.082f);
            float phase=fmodf(t*baseFreq,1.0f);
            float square = (phase<0.5f)?1.0f:-1.0f;
            float saw    = (2.0f*phase-1.0f);
            float s=(0.55f*saw+0.45f*square)*0.42f*env;
            data[i]=(short)(s*32767);
        }
        Wave w = (Wave){ .data=data, .frameCount=samples, .sampleRate=sr, .sampleSize=16, .channels=1};
        for (int i = 0; i < TITLE_POOL_SIZE; i++) {
            sfxTitleHoverPool[i] = LoadSoundFromWave(w);
            SetSoundVolume(sfxTitleHoverPool[i], 0.85f);
        }
        UnloadWave(w);
    }
    SetSoundVolume(sfxHover, 0.75f);
    SetSoundVolume(sfxClick, 0.95f);

    // Init voices
    memset(voices, 0, sizeof(voices));
    // Create realtime audio stream and set callback
    synthStream = LoadAudioStream(SYNTH_SAMPLE_RATE, 32, SYNTH_CHANNELS);
    SetAudioStreamCallback(synthStream, SynthCallback);
    PlayAudioStream(synthStream);
}

void CloseAudioSystem() {
    UnloadSound(sfxHover);
    UnloadSound(sfxClick);
    for (int i = 0; i < TITLE_POOL_SIZE; i++) UnloadSound(sfxTitleHoverPool[i]);
    StopAudioStream(synthStream);
    UnloadAudioStream(synthStream);
    CloseAudioDevice();
}

void PlaySynthSound(const char* type) {
    // Find free voice
    for (int i = 0; i < MAX_VOICES; i++) {
        if (!voices[i].active) {
            SynthVoice* v = &voices[i];
            v->active = true;
            v->phase = 0.0f;
            v->volume = 0.9f;
            if (strcmp(type, "Shoot") == 0) {
                v->type = WAVE_SAW;
                v->freq = 800.0f;
                v->decayPerSample = 0.9985f; // rápido
                v->slidePerSample = -3.5f;   // cae agresivo
            } else if (strcmp(type, "Explode") == 0) {
                v->type = WAVE_NOISE;
                v->freq = 200.0f;
                v->decayPerSample = 0.9995f; // lento
                v->slidePerSample = -0.5f;
            } else if (strcmp(type, "Dash") == 0) {
                v->type = WAVE_SINE;
                v->freq = 600.0f;
                v->decayPerSample = 0.9990f;
                v->slidePerSample = 1.5f; // pequeño up-chirp
                v->volume = 0.4f;
            } else {
                // default beep
                v->type = WAVE_SINE;
                v->freq = 440.0f;
                v->decayPerSample = 0.9990f;
                v->slidePerSample = 0.0f;
                v->volume = 0.5f;
            }
            break;
        }
    }
}

void PlayUIHover() {
    PlaySound(sfxHover);
}

void PlayUIClick() {
    PlaySound(sfxClick);
}

void PlayTitleHover() {
    // Overlap by cycling through a small pool
    PlaySound(sfxTitleHoverPool[sfxTitleHoverIndex]);
    sfxTitleHoverIndex = (sfxTitleHoverIndex + 1) % TITLE_POOL_SIZE;
}