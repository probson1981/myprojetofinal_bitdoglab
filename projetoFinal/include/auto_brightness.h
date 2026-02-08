#ifndef AUTO_BRIGHTNESS_H
#define AUTO_BRIGHTNESS_H

#include <stdint.h>
#include "hardware/i2c.h"

// Configurações do controle automático
typedef struct {
    // Faixa de lux (calibrar!)
    float   lux_min;          // ex: 5.0
    float   lux_max;          // ex: 800.0

    // Limites de brilho (%)
    uint8_t brightness_min;   // ex: 5
    uint8_t brightness_max;   // ex: 60

    // Suavização EMA (0..1)
    float   alpha;            // ex: 0.25

    // Período de atualização (ms)
    uint32_t update_ms;       // ex: 100

    // I2C/pinos
    i2c_inst_t *i2c;          // ex: i2c0
    uint sda_pin;             // ex: 0
    uint scl_pin;             // ex: 1
    uint32_t i2c_baud_hz;     // ex: 100000
} auto_brightness_config_t;

// Carrega valores padrão (para você ajustar depois, se quiser)
void auto_brightness_config_default(auto_brightness_config_t *cfg);

// Copia a config para uso da task
void auto_brightness_set_config(const auto_brightness_config_t *cfg);

// Funções utilitárias (expostas)
uint8_t lux_to_brightness_percent_inverse(float lux,
                                         float lux_min,
                                         float lux_max,
                                         uint8_t brightness_min,
                                         uint8_t brightness_max);

float ema_filter(float prev, float x, float alpha);

// Task FreeRTOS: lê BH1750 e ajusta brilho da matriz inversamente ao lux
void task_auto_brightness(void *pvParameters);

#endif // AUTO_BRIGHTNESS_H
