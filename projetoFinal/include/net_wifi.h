#ifndef NET_WIFI_H
#define NET_WIFI_H

/**
 * @file net_wifi.h
 * @brief Inicialização do Wi-Fi (Pico W / cyw43).
 */

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Inicializa Wi-Fi, entra em STA mode e conecta com timeout.
 * @return true se conectado; false caso falhe.
 */
bool net_wifi_init_and_connect(void);

#ifdef __cplusplus
}
#endif

#endif // NET_WIFI_H
