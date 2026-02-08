#ifndef MQTT_APP_H
#define MQTT_APP_H

/**
 * @file mqtt_app.h
 * @brief Cliente MQTT (lwIP RAW) com suporte a:
 *  - conexão/reconexão
 *  - subscribe em tópico de comando
 *  - publish de telemetria com QoS/ACK (opcional)
 *
 * Observação: callbacks do lwIP não devem usar APIs do FreeRTOS.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "FreeRTOS.h"
#include "task.h"

#include "lwip/apps/mqtt.h"
#include "lwip/ip_addr.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    mqtt_client_t *client;

    // conexão
    volatile bool connected;
    volatile bool connecting; 
    volatile bool need_subscribe;
    volatile bool conn_event;
    volatile int  conn_status;

    // comando RX (preenchido em callbacks)
    volatile bool     cmd_ready;
    volatile uint16_t cmd_len;
    char cmd_buf[256];
    char cmd_topic[96];

    // publish ACK
    volatile bool pub_done;
    volatile err_t pub_err;

    // identificação / tópicos
    char device_id[32];
    char topic_tele[96];
    char topic_cmd[96];

    // controle de reconexão / seq
    TickType_t last_attempt;
    uint32_t   last_sent_seq;
} mqtt_app_t;

/**
 * @brief Inicializa o módulo MQTT e gera device_id + tópicos.
 * @param m          Estrutura do módulo.
 * @param device_id  Buffer com ID (ou NULL para gerar via pico unique id).
 */
void mqtt_app_init(mqtt_app_t *m, const char *device_id);

/**
 * @brief Faz 1 tentativa de conexão MQTT (DNS + connect).
 * @return true se chamada de connect retornou ERR_OK; false caso contrário.
 */
bool mqtt_app_connect_once(mqtt_app_t *m);

/**
 * @brief Faz subscribe no tópico de comando.
 * @return true se subscribe retornou ERR_OK; false caso contrário.
 */
bool mqtt_app_subscribe_cmd(mqtt_app_t *m);

/**
 * @brief Publica um frame (telemetria) no tópico de telemetry.
 * @param m  módulo.
 * @param f  ponteiro para frame.
 * @return true se publish ok (e ACK ok quando QoS>0); false caso falhe.
 */
bool mqtt_app_publish_frame(mqtt_app_t *m, const void *f, size_t f_sz, uint32_t seq, TickType_t tick);

/**
 * @brief Se existir comando pendente, copia topic/payload e limpa flag.
 * @return true se havia comando; false caso contrário.
 */
bool mqtt_app_take_cmd(mqtt_app_t *m, char *topic_out, size_t topic_sz, char *payload_out, size_t payload_sz);

#ifdef __cplusplus
}
#endif

#endif // MQTT_APP_H
