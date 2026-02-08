#include <stdint.h>
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "ws2812.pio.h"
#include "matrix_led_lib.h"

#define max_value_light 50

static inline float clampf(float x, float a, float b) {
    return (x < a) ? a : (x > b) ? b : x;
}

static inline uint8_t lux_to_percent(float lux, float lux_min, float lux_max) {
    lux = clampf(lux, lux_min, lux_max);
    float t = (lux - lux_min) / (lux_max - lux_min); // 0..1
    return (uint8_t)(t * 100.0f + 0.5f);
}

// -------------------------
// Versões _ex (com out_percent)
// -------------------------
uint32_t matrix_set_brightness_percent_ex(uint8_t percent_0_100, uint8_t *out_percent_0_100)
{
    if (percent_0_100 > 100) percent_0_100 = 100;
    if (out_percent_0_100) *out_percent_0_100 = percent_0_100;

    uint8_t v = (uint8_t)((percent_0_100 * (uint32_t)max_value_light + 50) / 100);
    return ((uint32_t)v << 16) | ((uint32_t)v << 8) | (uint32_t)v; // GRB
}

uint32_t matrix_set_brightness_percent_inverse_ex(uint8_t percent_0_100, uint8_t *out_percent_0_100)
{
    if (percent_0_100 > 100) percent_0_100 = 100;

    uint8_t inv = (uint8_t)(100 - percent_0_100);
    if (out_percent_0_100) *out_percent_0_100 = inv;

    uint8_t v = (uint8_t)((inv * (uint32_t)max_value_light + 50) / 100);
    return ((uint32_t)v << 16) | ((uint32_t)v << 8) | (uint32_t)v; // GRB
}

uint32_t matrix_set_brightness_percent_inverse_from_lux_ex(float lux, uint8_t *out_percent_0_100)
{
    const float LUX_MIN = 150.0f;
    const float LUX_MAX = 400.0f;

    uint8_t percent = lux_to_percent(lux, LUX_MIN, LUX_MAX); // 0..100
    uint8_t inv     = (uint8_t)(100 - percent);

    if (out_percent_0_100) *out_percent_0_100 = inv;

    uint8_t v = (uint8_t)((inv * (uint32_t)max_value_light + 50) / 100);

    printf("lux=%.1f  percent=%u%%  inv=%u%%  v=%u\n", lux, percent, inv, v);

    return ((uint32_t)v << 16) | ((uint32_t)v << 8) | (uint32_t)v; // GRB
}

// -------------------------
// Wrappers (assinatura antiga)
// -------------------------
uint32_t matrix_set_brightness_percent(uint8_t percent_0_100)
{
    return matrix_set_brightness_percent_ex(percent_0_100, NULL);
}

uint32_t matrix_set_brightness_percent_inverse(uint8_t percent_0_100)
{
    return matrix_set_brightness_percent_inverse_ex(percent_0_100, NULL);
}

uint32_t matrix_set_brightness_percent_inverse_from_lux(float lux)
{
    return matrix_set_brightness_percent_inverse_from_lux_ex(lux, NULL);
}

// -------------------------
// Utilitários
// -------------------------
uint8_t matrix_percent_from_grb(uint32_t grb)
{
    uint8_t g = (uint8_t)((grb >> 16) & 0xFF); // G (assumindo r=g=b)
    uint32_t p = (g * 100u + (max_value_light / 2u)) / (uint32_t)max_value_light;
    if (p > 100u) p = 100u;
    return (uint8_t)p;
}

void put_pixel(PIO pio, uint sm, uint32_t pixel_grb)
{
    pio_sm_put_blocking(pio, sm, pixel_grb << 8u);
}

void matrix_init(PIO pio, uint sm, uint pin)
{
    uint offset = pio_add_program(pio, &ws2812_program);
    ws2812_program_init(pio, sm, offset, pin, 800000.0f, false);
}
