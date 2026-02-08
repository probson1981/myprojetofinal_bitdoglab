#include "mqtt_app.h"

#include <stdio.h>
#include <string.h>

#include "pico/unique_id.h"
#include "pico/cyw43_arch.h"

#include "app_config.h"
#include "net_dns.h"

// ================================
// Helpers
// ================================
static void get_device_id_string(char *out, size_t out_sz)
{
    pico_get_unique_board_id_string(out, out_sz);
}

static void make_topics(mqtt_app_t *m, const char *device_id)
{
    snprintf(m->topic_tele, sizeof(m->topic_tele), "%s/%s/telemetry", APP_TOPIC_PREFIX, device_id);
    snprintf(m->topic_cmd,  sizeof(m->topic_cmd),  "%s/%s/cmd",       APP_TOPIC_PREFIX, device_id);
}

// ================================
// Callbacks (lwIP)
// ================================
static void mqtt_connection_cb(mqtt_client_t *client, void *arg, mqtt_connection_status_t status)
{
    (void)client;
    mqtt_app_t *m = (mqtt_app_t*)arg;

    m->connecting = false; // <<< FIX: terminou a tentativa (sucesso ou falha)

    bool ok = (status == MQTT_CONNECT_ACCEPTED);
    m->connected = ok;

    if (ok) {
        m->need_subscribe = true; // assina cmd após conectar
    } else {
        m->need_subscribe = false;
    }

    m->conn_status = (int)status;
    m->conn_event = true;
}

static void mqtt_incoming_publish_cb(void *arg, const char *topic, u32_t tot_len)
{
    (void)tot_len;
    mqtt_app_t *m = (mqtt_app_t*)arg;

    if (m->cmd_ready) return;

    snprintf(m->cmd_topic, sizeof(m->cmd_topic), "%s", topic);

    m->cmd_len = 0;
    memset(m->cmd_buf, 0, sizeof(m->cmd_buf));
}

static void mqtt_incoming_data_cb(void *arg, const u8_t *data, u16_t len, u8_t flags)
{
    mqtt_app_t *m = (mqtt_app_t*)arg;
    if (m->cmd_ready) return;

    uint16_t can = (uint16_t)(sizeof(m->cmd_buf) - 1 - m->cmd_len);
    uint16_t n = (len < can) ? len : can;

    if (n > 0) {
        memcpy(&m->cmd_buf[m->cmd_len], data, n);
        m->cmd_len += n;
        m->cmd_buf[m->cmd_len] = '\0';
    }

    if (flags & MQTT_DATA_FLAG_LAST) {
        m->cmd_ready = true;
    }
}

static void mqtt_pub_cb(void *arg, err_t err)
{
    mqtt_app_t *m = (mqtt_app_t*)arg;
    m->pub_err  = err;
    m->pub_done = true;
}

// ================================
// API
// ================================
void mqtt_app_init(mqtt_app_t *m, const char *device_id)
{
    memset(m, 0, sizeof(*m));

    if (device_id && device_id[0]) {
        snprintf(m->device_id, sizeof(m->device_id), "%s", device_id);
    } else {
        get_device_id_string(m->device_id, sizeof(m->device_id));
    }

    make_topics(m, m->device_id);

    m->client = mqtt_client_new();

    m->connected = false;
    m->connecting = false;      // <<< NOVO
    m->need_subscribe = false;

    m->conn_event = false;
    m->conn_status = -999;

    m->last_attempt = 0;
    m->last_sent_seq = 0;
}

bool mqtt_app_connect_once(mqtt_app_t *m)
{
    if (!m->client) {
        m->client = mqtt_client_new();
        if (!m->client) return false;
    }

    // >>> FIX CRÍTICO: não chame connect de novo se já está conectado/connecting
    if (m->connected) return true;
    if (m->connecting) return true;

    // checagem extra (estado interno do lwIP)
    if (mqtt_client_is_connected(m->client)) {
        m->connected = true;
        m->need_subscribe = true;
        return true;
    }

    ip_addr_t broker_addr;
    printf("MQTT: resolving %s...\n", APP_MQTT_BROKER_HOST);
    if (!net_dns_resolve_host_to_ip(APP_MQTT_BROKER_HOST, &broker_addr, 5000)) {
        printf("MQTT: DNS failed\n");
        return false;
    }

    struct mqtt_connect_client_info_t ci;
    memset(&ci, 0, sizeof(ci));

    ci.client_id  = m->device_id;
    ci.keep_alive = APP_MQTT_KEEPALIVE_S;
    ci.client_user = NULL;
    ci.client_pass = NULL;

    m->connected = false;
    m->connecting = true;  // <<< NOVO: a partir daqui estamos aguardando callback

    cyw43_arch_lwip_begin();
    err_t e = mqtt_client_connect(m->client, &broker_addr, APP_MQTT_BROKER_PORT,
                                 mqtt_connection_cb, m, &ci);
    cyw43_arch_lwip_end();

    // ERR_OK = iniciou tentativa
    if (e == ERR_OK) {
        printf("MQTT: connect started\n");
        return true;
    }

    // ERR_ISCONN (-10) ou ERR_ALREADY (-9) => ignore como “já em andamento”
    if (e == ERR_ISCONN || e == ERR_ALREADY) {
        printf("MQTT: connect already in progress/connected (err=%d)\n", (int)e);
        return true;
    }

    // outro erro: limpa connecting para permitir retry
    m->connecting = false;
    printf("MQTT: mqtt_client_connect() err=%d\n", (int)e);
    return false;
}

bool mqtt_app_subscribe_cmd(mqtt_app_t *m)
{
    if (!m->client || !m->connected) return false;

    cyw43_arch_lwip_begin();
    mqtt_set_inpub_callback(m->client, mqtt_incoming_publish_cb, mqtt_incoming_data_cb, m);
    cyw43_arch_lwip_end();

    cyw43_arch_lwip_begin();
    err_t e = mqtt_subscribe(m->client, m->topic_cmd, 1, NULL, NULL);
    cyw43_arch_lwip_end();

    printf("MQTT: subscribe cmd (%s) err=%d\n", m->topic_cmd, (int)e);
    return (e == ERR_OK);
}

bool mqtt_app_publish_frame(mqtt_app_t *m, const void *payload, size_t payload_sz, uint32_t seq, TickType_t tick)
{
    (void)tick;

    if (!m->client || !m->connected) return false;

    m->pub_done = false;
    m->pub_err  = ERR_INPROGRESS;

    cyw43_arch_lwip_begin();
    err_t e = mqtt_publish(m->client, m->topic_tele,
                           (const char*)payload,
                           (u16_t)payload_sz,
                           APP_MQTT_QOS, APP_MQTT_RETAIN,
                           mqtt_pub_cb, m);
    cyw43_arch_lwip_end();

    if (e != ERR_OK) return false;

    if (APP_MQTT_QOS > 0) {
        TickType_t t0 = xTaskGetTickCount();
        while (!m->pub_done) {
            if ((xTaskGetTickCount() - t0) > pdMS_TO_TICKS(APP_MQTT_ACK_TIMEOUT_MS)) {
                return false;
            }
            vTaskDelay(pdMS_TO_TICKS(5));
        }
        if (m->pub_err == ERR_OK) {
            m->last_sent_seq = seq;
            return true;
        }
        return false;
    }

    m->last_sent_seq = seq;
    return true;
}

bool mqtt_app_take_cmd(mqtt_app_t *m, char *topic_out, size_t topic_sz, char *payload_out, size_t payload_sz)
{
    if (!m->cmd_ready) return false;

    if (topic_out && topic_sz) {
        snprintf(topic_out, topic_sz, "%s", m->cmd_topic);
    }
    if (payload_out && payload_sz) {
        snprintf(payload_out, payload_sz, "%s", m->cmd_buf);
    }

    m->cmd_ready = false;
    return true;
}
        