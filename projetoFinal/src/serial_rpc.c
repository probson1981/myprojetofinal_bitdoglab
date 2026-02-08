#include "serial_rpc.h"

#include <stdio.h>
#include <string.h>

#include "pico/stdlib.h"

#include "app_config.h"
#include "app_ctx.h"
#include "json_simple.h"
#include "matrix_control.h"

/**
 * @brief Envia hello no protocolo SerialRPC.
 */
static void serial_send_hello(const app_ctx_t *ctx, bool authed)
{
    printf("{\"op\":\"hello\",\"device\":\"%s\",\"fw\":\"pico-serial-v1\",\"need_auth\":%s}\n",
           ctx->device_id,
           authed ? "false" : "true");
}

/**
 * @brief Envia resposta de autenticação.
 */
static void serial_send_auth(bool ok)
{
    printf("{\"op\":\"auth\",\"ok\":%s}\n", ok ? "true" : "false");
}

/**
 * @brief Envia erro no protocolo SerialRPC.
 */
static void serial_send_err(const char *msg)
{
    printf("{\"op\":\"err\",\"msg\":\"%s\"}\n", msg ? msg : "");
}

/**
 * @brief Envia ack no protocolo SerialRPC.
 */
static void serial_send_ack(const char *msg)
{
    printf("{\"op\":\"ack\",\"ok\":true,\"msg\":\"%s\"}\n", msg ? msg : "");
}

/**
 * @brief Lê uma linha (não-bloqueante) do stdio via getchar_timeout_us.
 *
 * Observação: considera linhas terminadas com '\n'. Ignora '\r'.
 */
static bool serial_read_line(char *out, size_t out_sz)
{
    static char buf[256];
    static size_t idx = 0;

    int ch = getchar_timeout_us(0);
    if (ch == PICO_ERROR_TIMEOUT) return false;

    if (ch == '\r') return false;

    if (ch == '\n') {
        buf[idx] = '\0';
        snprintf(out, out_sz, "%s", buf);
        idx = 0;
        return true;
    }

    if (idx + 1 < sizeof(buf)) {
        buf[idx++] = (char)ch;
    }
    return false;
}

void vTaskSerialRpc(void *pvParameters)
{
    app_ctx_t *ctx = (app_ctx_t*)pvParameters;

    bool authed = false;
    uint32_t last_seq = 0;
    TickType_t last_tele = 0;

    printf("{\"op\":\"info\",\"msg\":\"SerialRPC up\"}\n");
    serial_send_hello(ctx, authed);

    char line[256];

    for (;;)
    {
        // 1) RX: processa linhas JSON do PC
        if (serial_read_line(line, sizeof(line))) {

            if (line[0] == '\0') { vTaskDelay(pdMS_TO_TICKS(5)); continue; }

            char op[16] = {0};
            if (json_get_string(line, "op", op, sizeof(op))) {

                if (strcmp(op, "hello") == 0) {
                    serial_send_hello(ctx, authed);
                }
                else if (strcmp(op, "logout") == 0) {
                    authed = false;
                    serial_send_ack("logout ok");
                    serial_send_hello(ctx, authed);
                }
                else if (strcmp(op, "auth") == 0) {
                    char pass[64] = {0};
                    bool got = json_get_string(line, "password", pass, sizeof(pass)) ||
                               json_get_string(line, "pass", pass, sizeof(pass));

                    if (!got) {
                        serial_send_auth(false);
                    } else {
                        bool ok = (strcmp(pass, APP_SERIAL_ACCESS_PASSWORD) == 0);
                        authed = ok;
                        serial_send_auth(ok);
                        if (ok) serial_send_ack("auth ok");
                    }
                }
                else if (strcmp(op, "cmd") == 0) {
                    if (!authed) {
                        serial_send_err("not authenticated");
                    } else {
                        // reaproveita o mesmo payload para o parser de comandos
                        matrix_control_apply_cmd_payload(line);
                        serial_send_ack("cmd applied");
                    }
                }
                else {
                    serial_send_err("unknown op");
                }
            } else {
                // opcional: se não tem "op", trata como comando implícito (quando autenticado)
                if (authed && (strstr(line, "\"mode\"") || strstr(line, "\"brightness\"") || strstr(line, "\"percent\""))) {
                    matrix_control_apply_cmd_payload(line);
                    serial_send_ack("cmd applied (implicit)");
                } else {
                    serial_send_err("bad json");
                }
            }
        }

        // 2) TX: telemetria (somente se autenticado)
        if (authed) {
            TickType_t now = xTaskGetTickCount();

            if ((now - last_tele) >= pdMS_TO_TICKS(APP_SERIAL_TELE_PERIOD_MS)) {
                last_tele = now;

                sensor_frame_t fr;
                if (xQueuePeek(ctx->q_frame, &fr, 0) == pdPASS) {
                    if (fr.seq != last_seq) {
                        last_seq = fr.seq;

                        const char *mstr = (matrix_control_get_mode() == MATRIX_MODE_AUTO) ? "auto" : "manual";

                        printf("{\"op\":\"telemetry\",\"device\":\"%s\","
                               "\"lux\":%.2f,\"luxPercLum\":%.1f,"
                               "\"temp\":%.2f,\"hum\":%.2f,"
                               "\"seq\":%lu,\"t_ms\":%lu,"
                               "\"mode\":\"%s\",\"target\":%u,\"current\":%u}\n",
                               ctx->device_id,
                               (double)fr.lux,
                               (double)fr.luxPercLum,
                               (double)fr.temp,
                               (double)fr.hum,
                               (unsigned long)fr.seq,
                               (unsigned long)pdTICKS_TO_MS(fr.tick),
                               mstr,
                               (unsigned)matrix_control_get_target_percent(),
                               (unsigned)matrix_control_get_current_percent());
                    }
                }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
