#include "matrix_control.h"

#include <stdio.h>
#include <string.h>

#include "app_config.h"
#include "json_simple.h"

/**
 * @brief Estado interno do controlador.
 */
static volatile matrix_mode_t g_mode = MATRIX_MODE_AUTO;
static volatile uint8_t g_target_percent = 100;  // alvo MANUAL (0..100)
static volatile uint8_t g_current_percent = 100; // aplicado (com fade)

/**
 * @brief Saturação para [0..100].
 */
static inline uint8_t clamp_u8_0_100(int v)
{
    if (v < 0) v = 0;
    if (v > 100) v = 100;
    return (uint8_t)v;
}

/**
 * @brief Avança o valor atual em direção ao alvo em ~10 passos.
 */
static inline uint8_t step_towards_u8(uint8_t cur, uint8_t tgt)
{
    if (cur == tgt) return cur;

    uint8_t delta = (cur > tgt) ? (uint8_t)(cur - tgt) : (uint8_t)(tgt - cur);
    uint8_t step  = (uint8_t)((delta + 9u) / 10u); // ~10 passos
    if (step < 1u) step = 1u;

    if (cur < tgt) {
        uint16_t n = (uint16_t)cur + (uint16_t)step;
        return (n > tgt) ? tgt : (uint8_t)n;
    } else {
        int n = (int)cur - (int)step;
        return (n < (int)tgt) ? tgt : (uint8_t)n;
    }
}

/**
 * @brief Saturação float.
 */
static inline float clampf(float x, float a, float b)
{
    return (x < a) ? a : (x > b) ? b : x;
}

/**
 * @brief Mapeia lux (faixa) para percent 0..100.
 */
static inline uint8_t lux_to_percent(float luxv, float lux_min, float lux_max)
{
    luxv = clampf(luxv, lux_min, lux_max);
    float t = (luxv - lux_min) / (lux_max - lux_min); // 0..1
    return (uint8_t)(t * 100.0f + 0.5f);
}

/**
 * @brief Converte lux filtrado para percentual de brilho, invertido.
 *
 * Ex.: lux alto -> percent baixo, lux baixo -> percent alto.
 */
static inline uint8_t auto_percent_inverse_from_lux(float luxv)
{
    uint8_t p = lux_to_percent(luxv, APP_LUX_MIN, APP_LUX_MAX); // 0..100
    return (uint8_t)(100u - p);
}

void matrix_control_init(void)
{
    g_mode = MATRIX_MODE_AUTO;
    g_target_percent = 100;
    g_current_percent = 100;
}

void matrix_control_apply_cmd_payload(const char *payload)
{
    if (!payload || !payload[0]) return;

    // mode
    char mode[16] = {0};
    if (json_get_string(payload, "mode", mode, sizeof(mode))) {
        if (strcmp(mode, "auto") == 0) {
            g_mode = MATRIX_MODE_AUTO;
            printf("[CMD] mode=auto\n");
        } else if (strcmp(mode, "manual") == 0) {
            g_mode = MATRIX_MODE_MANUAL;
            printf("[CMD] mode=manual\n");
        } else {
            printf("[CMD] mode desconhecido: %s\n", mode);
        }
    }

    // percent
    int v = 0;
    if (json_get_int(payload, "matrixPercent", &v) ||
        json_get_int(payload, "brightness", &v) ||
        json_get_int(payload, "percent", &v)) {

        uint8_t p = clamp_u8_0_100(v);
        g_target_percent = p;

        // Se mandou percent sem declarar mode, assume MANUAL (útil para controle rápido).
        if (g_mode != MATRIX_MODE_MANUAL) {
            g_mode = MATRIX_MODE_MANUAL;
            printf("[CMD] assumindo modo manual\n");
        }
        printf("[CMD] target=%u%%\n", (unsigned)p);
    }
}

uint8_t matrix_control_update_from_lux(float lux_filtered)
{
    // define alvo conforme modo
    uint8_t desired = 0;
    if (g_mode == MATRIX_MODE_MANUAL) {
        desired = g_target_percent;
    } else {
        desired = auto_percent_inverse_from_lux(lux_filtered);
    }

    // aplica fading
    uint8_t cur = g_current_percent;
    cur = step_towards_u8(cur, desired);
    g_current_percent = cur;

    return cur;
}

matrix_mode_t matrix_control_get_mode(void)
{
    return g_mode;
}

uint8_t matrix_control_get_target_percent(void)
{
    return g_target_percent;
}

uint8_t matrix_control_get_current_percent(void)
{
    return g_current_percent;
}
