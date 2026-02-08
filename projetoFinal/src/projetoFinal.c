#include <stdio.h>
#include <string.h>

#include "pico/stdlib.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "hardware/i2c.h"

#include "app_config.h"
#include "app_ctx.h"
#include "net_wifi.h"
#include "mqtt_app.h"
#include "matrix_control.h"
#include "app_tasks.h"
#include "serial_rpc.h"

/**
 * @brief Inicializa I2C0 (sensores BH1750 e AHT10).
 */
static void app_i2c0_init(void)
{
    i2c_init(APP_I2C0_PORT, APP_I2C0_BAUD_HZ);
    gpio_set_function(APP_I2C0_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(APP_I2C0_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(APP_I2C0_SDA_PIN);
    gpio_pull_up(APP_I2C0_SCL_PIN);
}

/**
 * @brief Cria filas (queues) com tamanho 1 e overwrite.
 */
static void app_queues_init(app_ctx_t *ctx)
{
    ctx->q_lux   = xQueueCreate(1, sizeof(float));
    ctx->q_perc  = xQueueCreate(1, sizeof(float));
    ctx->q_temp  = xQueueCreate(1, sizeof(float));
    ctx->q_hum   = xQueueCreate(1, sizeof(float));
    ctx->q_frame = xQueueCreate(1, sizeof(sensor_frame_t));

    configASSERT(ctx->q_lux && ctx->q_perc && ctx->q_temp && ctx->q_hum && ctx->q_frame);
}

/**
 * @brief main - ponto de entrada.
 */
int main(void)
{
    stdio_init_all();
    sleep_ms(1500);

    // Wi-Fi
    if (!net_wifi_init_and_connect()) {
        return -1;
    }

    // contexto
    static app_ctx_t ctx;
    memset(&ctx, 0, sizeof(ctx));

    // inicializações básicas
    app_queues_init(&ctx);

#if APP_USE_I2C0_MUTEX
    ctx.i2c0_mutex = xSemaphoreCreateMutex();
    configASSERT(ctx.i2c0_mutex != NULL);
#endif

    app_i2c0_init();

    // controle de brilho / comandos
    matrix_control_init();

    // MQTT init (gera device_id e tópicos)
    mqtt_app_init(&ctx.mqtt, NULL);
    snprintf(ctx.device_id, sizeof(ctx.device_id), "%s", ctx.mqtt.device_id);

    // tasks
    xTaskCreate(vTaskLuminos,      "Luminos",     4096, &ctx, 1, NULL);
    xTaskCreate(vTaskTempUmidade,  "TempUmidade", 4096, &ctx, 1, NULL);
    xTaskCreate(vTaskDisplay,      "Display",     4096, &ctx, 2, NULL);

    BaseType_t ok = xTaskCreate(vTaskMqtt, "Mqtt", 8192, &ctx, 2, &ctx.task_mqtt);
    configASSERT(ok == pdPASS);
    configASSERT(ctx.task_mqtt != NULL);

    BaseType_t ok2 = xTaskCreate(vTaskSerialRpc, "SerialRPC", 4096, &ctx, 2, NULL);
    configASSERT(ok2 == pdPASS);

    vTaskStartScheduler();

    while (true) { }
}
