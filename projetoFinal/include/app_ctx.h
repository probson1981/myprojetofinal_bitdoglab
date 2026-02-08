#ifndef APP_CTX_H
#define APP_CTX_H

/**
 * @file app_ctx.h
 * @brief Estruturas compartilhadas entre tasks (queues, handles, IDs).
 */

#include <stdint.h>

#include "app_config.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#if APP_USE_I2C0_MUTEX
#include "semphr.h"
#endif

#include "mqtt_app.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Frame agregado de sensores para telemetria / display.
 */
typedef struct {
    float lux;
    float luxPercLum;
    float temp;
    float hum;
    uint32_t seq;
    TickType_t tick;
} sensor_frame_t;

/**
 * @brief Contexto da aplicação (passado para tasks via pvParameters).
 */
typedef struct app_ctx {
    // IDs/tópicos
    char device_id[32];

    // filas (size=1, overwrite)
    QueueHandle_t q_frame;
    QueueHandle_t q_lux;
    QueueHandle_t q_temp;
    QueueHandle_t q_hum;
    QueueHandle_t q_perc;

#if APP_USE_I2C0_MUTEX
    SemaphoreHandle_t i2c0_mutex;
#endif

    // tasks
    TaskHandle_t task_mqtt;

    // MQTT state
    mqtt_app_t mqtt;
} app_ctx_t;

#ifdef __cplusplus
}
#endif

#endif // APP_CTX_H
