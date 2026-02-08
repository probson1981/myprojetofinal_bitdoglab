#include "net_dns.h"

#include <stdio.h>

#include "pico/cyw43_arch.h"

#include "FreeRTOS.h"
#include "task.h"

#include "lwip/dns.h"
#include "lwip/err.h"

#ifndef LWIP_DNS
#error "lwipopts.h nao esta sendo usado (LWIP_DNS indefinido)."
#endif
#if LWIP_DNS != 1
#error "LWIP_DNS precisa ser 1."
#endif

// Estado de resolução (callbacks do lwIP não podem usar FreeRTOS diretamente)
static volatile bool g_dns_done = false;
static volatile err_t g_dns_err = ERR_VAL;
static ip_addr_t g_dns_ip;

/**
 * @brief Callback do lwIP chamado quando DNS completa.
 */
static void dns_found_cb(const char *name, const ip_addr_t *ipaddr, void *arg)
{
    (void)name; (void)arg;

    if (ipaddr) {
        g_dns_ip  = *ipaddr;
        g_dns_err = ERR_OK;
    } else {
        g_dns_err = ERR_VAL;
    }
    g_dns_done = true;
}

bool net_dns_resolve_host_to_ip(const char *host, ip_addr_t *out, uint32_t timeout_ms)
{
    g_dns_done = false;
    g_dns_err  = ERR_INPROGRESS;

    cyw43_arch_lwip_begin();
    err_t e = dns_gethostbyname(host, &g_dns_ip, dns_found_cb, NULL);
    cyw43_arch_lwip_end();

    if (e == ERR_OK) { // cache
        *out = g_dns_ip;
        return true;
    }
    if (e != ERR_INPROGRESS) {
        return false;
    }

    TickType_t t0 = xTaskGetTickCount();
    while (!g_dns_done) {
        if ((xTaskGetTickCount() - t0) > pdMS_TO_TICKS(timeout_ms)) return false;
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    if (g_dns_err == ERR_OK) {
        *out = g_dns_ip;
        return true;
    }
    return false;
}
