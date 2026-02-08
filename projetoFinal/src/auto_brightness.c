#include "auto_brightness.h"

#include "pico/stdlib.h"
#include "FreeRTOS.h"
#include "task.h"

#include "bh1750.h"
#include "matrix_led_lib.h"
#include <stdint.h>

// -------------------------
// Interno
// -------------------------
static inline float clampf(float x, float a, float b) {
    return (x < a) ? a : (x > b) ? b : x;
}

// Config global usada pela task
static auto_brightness_config_t g_cfg;

// -------------------------
// API
// -------------------------
void auto_brightness_config_default(auto_brightness_config_t *cfg)
{
    if (!cfg) return;

    cfg->lux_min        = 5.0f;
    cfg->lux_max        = 400.0f;
    cfg->brightness_min = 5;
    cfg->brightness_max = 60;
    cfg->alpha          = 0.25f;
    cfg->update_ms      = 100;

    cfg->i2c            = i2c0;
    cfg->sda_pin        = 0;
    cfg->scl_pin        = 1;
    cfg->i2c_baud_hz    = 100000;
}

void auto_brightness_set_config(const auto_brightness_config_t *cfg)
{
    if (!cfg) return;
    g_cfg = *cfg; // cópia
}

float ema_filter(float prev, float x, float alpha)
{
    // garante alpha 0..1
    alpha = clampf(alpha, 0.0f, 1.0f);
    return prev + alpha * (x - prev);
}

uint8_t lux_to_brightness_percent_inverse(float lux,
                                         float lux_min,
                                         float lux_max,
                                         uint8_t brightness_min,
                                         uint8_t brightness_max)
{
    // Evita divisão por zero / faixa inválida
    if (lux_max <= lux_min) {
        // fallback: brilho médio
        uint8_t mid = (uint8_t)((brightness_min + brightness_max) / 2);
        return (mid > 100) ? 100 : mid;
    }

    lux = clampf(lux, lux_min, lux_max);

    float t   = (lux - lux_min) / (lux_max - lux_min); // 0..1
    float inv = 1.0f - t;

    float b = (float)brightness_min + inv * (float)(brightness_max - brightness_min);

    b = clampf(b, 0.0f, 100.0f);
    return 20;//(uint8_t)(b + 0.5f);
}

