#ifndef MATRIX_LED_LIB_H
#define MATRIX_LED_LIB_H

#include <stdint.h>
#include "hardware/pio.h"

#ifdef __cplusplus
extern "C" {
#endif

// =========================
// Init / envio
// =========================
void matrix_init(PIO pio, uint sm, uint pin);
void put_pixel(PIO pio, uint sm, uint32_t pixel_grb);

// =========================
// Brilho (retorna GRB)
// =========================
uint32_t matrix_set_brightness_percent(uint8_t percent_0_100);
uint32_t matrix_set_brightness_percent_inverse(uint8_t percent_0_100);
uint32_t matrix_set_brightness_percent_inverse_from_lux(float lux);

// Versões que também devolvem o percentual efetivo (0..100)
uint32_t matrix_set_brightness_percent_ex(uint8_t percent_0_100, uint8_t *out_percent_0_100);
uint32_t matrix_set_brightness_percent_inverse_ex(uint8_t percent_0_100, uint8_t *out_percent_0_100);
uint32_t matrix_set_brightness_percent_inverse_from_lux_ex(float lux, uint8_t *out_percent_0_100);

// =========================
// Utilitário: extrai % (0..100) de um GRB (assumindo r=g=b)
// =========================
uint8_t matrix_percent_from_grb(uint32_t grb);

#ifdef __cplusplus
}
#endif

#endif // MATRIX_LED_LIB_H
