#ifndef APP_CONFIG_H
#define APP_CONFIG_H

/**
 * @file app_config.h
 * @brief Configurações de projeto (Wi-Fi, MQTT, pinos, intervalos).
 *
 * Ajuste aqui SSID/PASSWORD, senha do SerialRPC e parâmetros de telemetria.
 */

#include "hardware/i2c.h"

// ==============================
// Wi-Fi
// ==============================
#define APP_WIFI_SSID     "PATRICIO MEGA WAVE"
#define APP_WIFI_PASSWORD "patricioalves"

// ==============================
// Serial RPC (USB-Serial JSON)
// ==============================
#define APP_SERIAL_ACCESS_PASSWORD "1234"   /**< Troque para uma senha forte. */
#define APP_SERIAL_TELE_PERIOD_MS  200u     /**< Período de telemetria via SerialRPC. */

// ==============================
// MQTT (HiveMQ Public - sem TLS / sem user/pass)
// ==============================
#define APP_MQTT_BROKER_HOST       "broker.hivemq.com"
#define APP_MQTT_BROKER_PORT       1883
#define APP_MQTT_KEEPALIVE_S       30u
#define APP_MQTT_QOS               1u
#define APP_MQTT_RETAIN            0u
#define APP_MQTT_ACK_TIMEOUT_MS    2000u

#define APP_TOPIC_PREFIX           "embarcatech"

// ==============================
// Hardware: WS2812 (matriz)
// ==============================
#define APP_LED_PIN                7u
#define APP_LED_COUNT              25u

// ==============================
// I2C0: BH1750 + AHT10
// ==============================
#define APP_I2C0_PORT              i2c0
#define APP_I2C0_SDA_PIN           0u
#define APP_I2C0_SCL_PIN           1u
#define APP_I2C0_BAUD_HZ           (100u * 1000u)

// ==============================
// I2C1: OLED SSD1306
// ==============================
#define APP_I2C1_PORT              i2c1
#define APP_I2C1_SDA_PIN           14u
#define APP_I2C1_SCL_PIN           15u
#define APP_I2C1_BAUD_HZ           (100u * 1000u)

// ==============================
// Auto-brightness: mapeamento lux -> brilho
// ==============================
#define APP_LUX_MIN                150.0f
#define APP_LUX_MAX                400.0f

// ==============================
// Sincronização do I2C0 entre tasks (recomendado)
// ==============================
#define APP_USE_I2C0_MUTEX         1

#endif // APP_CONFIG_H
