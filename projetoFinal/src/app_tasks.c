#include "app_tasks.h"

#include <stdio.h>
#include <string.h>

#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"

#include "hardware/i2c.h"
#include "hardware/pio.h"

#include "app_config.h"
#include "app_ctx.h"
#include "matrix_control.h"

#include "ws2812.pio.h"
#include "matrix_led_lib.h"
#include "bh1750.h"
#include "aht10.h"
#include "auto_brightness.h"
#include "ssd1306.h"

// ------------------------------------------------------------
// Helpers (mutex I2C0)
// ------------------------------------------------------------
#if APP_USE_I2C0_MUTEX
static inline void i2c0_lock(app_ctx_t *ctx)   { xSemaphoreTake(ctx->i2c0_mutex, portMAX_DELAY); }
static inline void i2c0_unlock(app_ctx_t *ctx) { xSemaphoreGive(ctx->i2c0_mutex); }
#else
static inline void i2c0_lock(app_ctx_t *ctx)   { (void)ctx; }
static inline void i2c0_unlock(app_ctx_t *ctx) { (void)ctx; }
#endif

// ------------------------------------------------------------
// Task: Luminosidade + WS2812
// ------------------------------------------------------------
/**
 * @brief Task que lê BH1750, filtra lux (EMA) e atualiza brilho da matriz WS2812.
 *
 * - Modo AUTO/MANUAL e fading são tratados por matrix_control_*.
 * - Publica nas filas: q_lux (lux bruto) e q_perc (percentual aplicado).
 */
void vTaskLuminos(void *pvParameters)
{
    app_ctx_t *ctx = (app_ctx_t*)pvParameters;

    auto_brightness_config_t cfg;
    auto_brightness_config_default(&cfg);
    // Mantém o mesmo "fallback" do seu código original.
    if (cfg.i2c == NULL) {
        auto_brightness_config_t def;
        auto_brightness_config_default(&def);
        auto_brightness_set_config(&def);
        cfg = def;
    } else {
        auto_brightness_set_config(&cfg);
    }

    vTaskDelay(pdMS_TO_TICKS(100));

    // BH1750 (I2C0)
    i2c0_lock(ctx);
    bh1750_init(APP_I2C0_PORT);
    i2c0_unlock(ctx);

    // WS2812 via PIO
    PIO pio = pio0;
    uint sm = 0;
    uint offset = pio_add_program(pio, &ws2812_program);
    ws2812_program_init(pio, sm, offset, APP_LED_PIN, 800000.0f, false);

    float lux_f = cfg.lux_max; // inicia filtro
    float lux = 0.0f;

    for (;;)
    {
        // lê lux (I2C0)
        i2c0_lock(ctx);
        lux = bh1750_read_lux(APP_I2C0_PORT);
        i2c0_unlock(ctx);

        // debug a cada ~10 ciclos
        static uint32_t cnt = 0;
        if ((cnt++ % 10) == 0) {
            printf("Lux: %.2f  mode=%s  target=%u  current=%u\n",
                   lux,
                   (matrix_control_get_mode() == MATRIX_MODE_AUTO) ? "AUTO" : "MANUAL",
                   (unsigned)matrix_control_get_target_percent(),
                   (unsigned)matrix_control_get_current_percent());
        }

        // filtro EMA
        lux_f = ema_filter(lux_f, lux, cfg.alpha);

        // atualiza controlador e aplica brilho
        uint8_t cur_percent = matrix_control_update_from_lux(lux_f);
        uint32_t color = matrix_set_brightness_percent(cur_percent);

        for (int i = 0; i < (int)APP_LED_COUNT; i++) {
            put_pixel(pio, sm, color);
        }

        // filas (overwrite)
        float perc = (float)cur_percent;
        xQueueOverwrite(ctx->q_lux, &lux);
        xQueueOverwrite(ctx->q_perc, &perc);

        vTaskDelay(pdMS_TO_TICKS(cfg.update_ms));
    }
}

// ------------------------------------------------------------
// Task: Temperatura e Umidade (AHT10)
// ------------------------------------------------------------
/**
 * @brief Task que inicializa e lê o sensor AHT10 (temp/umidade).
 *
 * Publica em q_temp e q_hum (overwrite). Leitura a cada ~2s.
 */
void vTaskTempUmidade(void *pvParameters)
{
    app_ctx_t *ctx = (app_ctx_t*)pvParameters;

    AHT10_Handle aht10 = {
        .iface = {
            .i2c_port  = APP_I2C0_PORT,
            .i2c_write = i2c_write,
            .i2c_read  = i2c_read,
            .delay_ms  = sleep_ms
        },
        .initialized = false
    };

    printf("Inicializando AHT10...\n");
    i2c0_lock(ctx);
    bool ok = AHT10_Init(&aht10);
    i2c0_unlock(ctx);

    if (!ok) {
        printf("Falha na inicialização do sensor AHT10!\n");
    }

    float temp = 0.0f, hum = 0.0f;

    while (true) {
        i2c0_lock(ctx);
        bool rd = AHT10_ReadTemperatureHumidity(&aht10, &temp, &hum);
        i2c0_unlock(ctx);

        if (rd) {
            static uint32_t cnt = 0;
            if ((cnt++ % 10) == 0) {
                printf("Temp: %.2f C | Umid: %.2f %%\n", temp, hum);
            }
        }

        xQueueOverwrite(ctx->q_temp, &temp);
        xQueueOverwrite(ctx->q_hum,  &hum);

        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

// ------------------------------------------------------------
// Task: Display SSD1306 (I2C1) + agrega frame
// ------------------------------------------------------------
/**
 * @brief Task do display OLED (SSD1306) e agregação do frame de telemetria.
 *
 * - Recebe temp e umidade (bloqueante).
 * - Pega lux e percentual via peek.
 * - Atualiza OLED e escreve o sensor_frame_t em q_frame.
 * - Notifica task MQTT para enviar telemetria.
 */
void vTaskDisplay(void *pvParameters)
{
    app_ctx_t *ctx = (app_ctx_t*)pvParameters;

    float lux_local = 0.0f, temp = 0.0f, hum = 0.0f, perc = 0.0f;
    char msg1[32];
    sensor_frame_t frame = {0};

    printf("Display...\n");

    // I2C1 para OLED
    i2c_init(APP_I2C1_PORT, APP_I2C1_BAUD_HZ);
    gpio_set_function(APP_I2C1_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(APP_I2C1_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(APP_I2C1_SDA_PIN);
    gpio_pull_up(APP_I2C1_SCL_PIN);

    ssd1306_init(APP_I2C1_PORT);
    ssd1306_clear();
    ssd1306_draw_string(0, 0, "Iniciando...");
    ssd1306_show();

    vTaskDelay(pdMS_TO_TICKS(30));
    ssd1306_clear();

    for (;;)
    {
        // bloqueia esperando temp/hum
        xQueueReceive(ctx->q_temp, &temp, portMAX_DELAY);
        xQueueReceive(ctx->q_hum,  &hum,  portMAX_DELAY);

        float tmp;
        if (xQueuePeek(ctx->q_lux, &tmp, 0) == pdPASS) {
            lux_local = tmp;
        }
        if (xQueuePeek(ctx->q_perc, &tmp, 0) == pdPASS) {
            perc = tmp;
        }

        // monta frame
        frame.lux  = lux_local;
        frame.luxPercLum = perc;
        frame.temp = temp;
        frame.hum  = hum;
        frame.seq++;
        frame.tick = xTaskGetTickCount();

        // OLED
        ssd1306_clear();
        ssd1306_draw_string(0, 0, "Lux, Temp. e Umidade");

        snprintf(msg1, sizeof(msg1), "Lux: %.2f Lx", lux_local);
        ssd1306_draw_string(0, 10, msg1);

        snprintf(msg1, sizeof(msg1), "Temp: %.2f C", temp);
        ssd1306_draw_string(0, 20, msg1);

        snprintf(msg1, sizeof(msg1), "Umid: %.2f %%", hum);
        ssd1306_draw_string(0, 30, msg1);

        snprintf(msg1, sizeof(msg1), "Perc.Lum: %.0f %%", perc);
        ssd1306_draw_string(0, 40, msg1);

        ssd1306_show();

        // fila do frame + notifica MQTT
        xQueueOverwrite(ctx->q_frame, &frame);

        if (ctx->task_mqtt) {
            xTaskNotifyGive(ctx->task_mqtt);
        }

        vTaskDelay(pdMS_TO_TICKS(200));
    }
}

// ------------------------------------------------------------
// Task: MQTT TX/RX
// ------------------------------------------------------------
/**
 * @brief Task que gerencia MQTT:
 *  - reconexão
 *  - subscribe
 *  - recepção de comando (/cmd)
 *  - publish de telemetria (/telemetry)
 */
void vTaskMqtt(void *pvParameters)
{
    app_ctx_t *ctx = (app_ctx_t*)pvParameters;

    printf("MQTT: client_id=%s\n", ctx->mqtt.device_id);
    printf("MQTT: tele=%s\n", ctx->mqtt.topic_tele);
    printf("MQTT: cmd =%s\n", ctx->mqtt.topic_cmd);

    // (Opcional) watchdog de "connecting"
    // Se ficar mais que X ms em connecting, reseta para tentar de novo
    const TickType_t CONNECTING_TIMEOUT = pdMS_TO_TICKS(15000);
    TickType_t connecting_since = 0;

    for (;;)
    {
        // Acorda por notificação do display (ou timeout para manutenção)
        ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(500));

        // evento de conexão
        if (ctx->mqtt.conn_event) {
            ctx->mqtt.conn_event = false;
            printf("MQTT: connection status=%d  connected=%d  connecting=%d\n",
                   ctx->mqtt.conn_status,
                   ctx->mqtt.connected,
                   ctx->mqtt.connecting);
        }

        // -------------------------
        // 1) Conexão / Reconexão
        // -------------------------
        if (!ctx->mqtt.connected)
        {
            // Se está "connecting", não chama connect de novo.
            if (ctx->mqtt.connecting) {
                if (connecting_since == 0) {
                    connecting_since = xTaskGetTickCount();
                } else {
                    // (Opcional) se "connecting" ficou preso tempo demais, reseta flags
                    if ((xTaskGetTickCount() - connecting_since) > CONNECTING_TIMEOUT) {
                        printf("MQTT: connecting timeout -> forcing retry\n");
                        ctx->mqtt.connecting = false;
                        // Se você quiser ser mais agressivo, pode também:
                        // cyw43_arch_lwip_begin();
                        // mqtt_disconnect(ctx->mqtt.client);
                        // cyw43_arch_lwip_end();
                    }
                }

                vTaskDelay(pdMS_TO_TICKS(200));
                continue;
            }

            // não está conectado e não está conectando: tenta com backoff
            connecting_since = 0;
            TickType_t now = xTaskGetTickCount();
            if ((now - ctx->mqtt.last_attempt) > pdMS_TO_TICKS(3000)) {
                ctx->mqtt.last_attempt = now;
                (void)mqtt_app_connect_once(&ctx->mqtt);
            }

            vTaskDelay(pdMS_TO_TICKS(200));
            continue;
        }

        // Se conectou, zera marcador de connecting_since
        connecting_since = 0;

        // -------------------------
        // 2) Subscribe após conectar
        // -------------------------
        if (ctx->mqtt.need_subscribe) {
            // NÃO zera antes. Só zera quando der certo.
            if (mqtt_app_subscribe_cmd(&ctx->mqtt)) {
                ctx->mqtt.need_subscribe = false;
            } else {
                // tenta de novo em próximas iterações
                printf("MQTT: subscribe failed, will retry\n");
            }
        }

        // -------------------------
        // 3) Comando RX
        // -------------------------
        char topic_local[96];
        char payload_local[256];

        if (mqtt_app_take_cmd(&ctx->mqtt,
                              topic_local, sizeof(topic_local),
                              payload_local, sizeof(payload_local))) {

            printf("CMD RX topic=%s payload=%s\n", topic_local, payload_local);

            matrix_control_apply_cmd_payload(payload_local);

            // blink LED onboard (feedback)
            cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
            vTaskDelay(pdMS_TO_TICKS(80));
            cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
        }

        // -------------------------
        // 4) TX Telemetria
        // -------------------------
        sensor_frame_t frame;
        if (xQueuePeek(ctx->q_frame, &frame, 0) != pdPASS) {
            continue;
        }
        if (frame.seq == ctx->mqtt.last_sent_seq) {
            continue;
        }

        char payload[240];
        int n = snprintf(payload, sizeof(payload),
            "{\"device\":\"%s\",\"lux\":%.2f,\"luxPercLum\":%.1f,"
            "\"temp\":%.2f,\"hum\":%.2f,\"seq\":%lu,\"t_ms\":%lu}",
            ctx->mqtt.device_id,
            (double)frame.lux,
            (double)frame.luxPercLum,
            (double)frame.temp,
            (double)frame.hum,
            (unsigned long)frame.seq,
            (unsigned long)pdTICKS_TO_MS(frame.tick)
        );
        if (n <= 0 || n >= (int)sizeof(payload)) {
            continue;
        }

        if (!mqtt_app_publish_frame(&ctx->mqtt,
                                    payload, (size_t)strlen(payload),
                                    frame.seq, frame.tick)) {
            // Falha no publish: força reconectar
            printf("MQTT: publish failed -> mark disconnected\n");
            ctx->mqtt.connected   = false;
            ctx->mqtt.connecting  = false;   // <<< importante para não travar em estado
            ctx->mqtt.need_subscribe = false;

            vTaskDelay(pdMS_TO_TICKS(300));
            continue;
        }

        // Se publish OK, segue loop
    }
}
