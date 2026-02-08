#include "net_wifi.h"

#include <stdio.h>

#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"

#include "app_config.h"

/**
 * @brief Inicializa e conecta ao Wi-Fi usando SSID/SENHA de app_config.h.
 */
bool net_wifi_init_and_connect(void)
{
    if (cyw43_arch_init()) {
        printf("Wi-Fi init failed\n");
        return false;
    }

    cyw43_arch_enable_sta_mode();

    printf("Connecting to Wi-Fi...\n");
    if (cyw43_arch_wifi_connect_timeout_ms(
            APP_WIFI_SSID,
            APP_WIFI_PASSWORD,
            CYW43_AUTH_WPA2_AES_PSK,
            30000)) {
        printf("failed to connect.\n");
        return false;
    }

    printf("Connected.\n");
    uint8_t *ip_address = (uint8_t*)&(cyw43_state.netif[0].ip_addr.addr);
    printf("IP address %d.%d.%d.%d\n", ip_address[0], ip_address[1], ip_address[2], ip_address[3]);
    return true;
}
