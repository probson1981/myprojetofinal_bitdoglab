#ifndef MATRIX_CONTROL_H
#define MATRIX_CONTROL_H

/**
 * @file matrix_control.h
 * @brief Controle do modo AUTO/MANUAL, alvo e fading de brilho para a matriz WS2812.
 */

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    MATRIX_MODE_AUTO = 0,
    MATRIX_MODE_MANUAL = 1
} matrix_mode_t;

/**
 * @brief Inicializa o controlador (modo AUTO, 100%).
 */
void matrix_control_init(void);

/**
 * @brief Aplica comando em JSON para atualizar modo e/ou percent.
 *
 * Payloads esperados (exemplos):
 *   {"mode":"auto"}
 *   {"mode":"manual","matrixPercent":80}
 * Também aceita chaves alternativas: "brightness" ou "percent".
 *
 * @param payload JSON como string.
 */
void matrix_control_apply_cmd_payload(const char *payload);

/**
 * @brief Atualiza o brilho (com fading) a partir do lux filtrado.
 *
 * - Em modo MANUAL: segue o target (0..100).
 * - Em modo AUTO: calcula percent invertido do lux (APP_LUX_MIN..APP_LUX_MAX).
 *
 * @param lux_filtered Lux após filtragem (EMA).
 * @return Percentual atual aplicado (0..100).
 */
uint8_t matrix_control_update_from_lux(float lux_filtered);

/**
 * @brief Retorna o modo atual.
 */
matrix_mode_t matrix_control_get_mode(void);

/**
 * @brief Retorna alvo manual (0..100).
 */
uint8_t matrix_control_get_target_percent(void);

/**
 * @brief Retorna percentual atualmente aplicado (0..100).
 */
uint8_t matrix_control_get_current_percent(void);

#ifdef __cplusplus
}
#endif

#endif // MATRIX_CONTROL_H
