#ifndef NET_DNS_H
#define NET_DNS_H

/**
 * @file net_dns.h
 * @brief Resolução DNS (lwIP) em modo threadsafe_background.
 */

#include <stdbool.h>
#include <stdint.h>

#include "lwip/ip_addr.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Resolve um hostname em ip_addr_t usando lwIP DNS.
 *
 * Esta função é bloqueante (com timeout) e usa cyw43_arch_lwip_begin/end.
 *
 * @param host        Hostname (ex.: "broker.hivemq.com").
 * @param out         Saída: IP resolvido.
 * @param timeout_ms  Timeout em milissegundos.
 * @return true se sucesso; false caso falhe/timeout.
 */
bool net_dns_resolve_host_to_ip(const char *host, ip_addr_t *out, uint32_t timeout_ms);

#ifdef __cplusplus
}
#endif

#endif // NET_DNS_H
