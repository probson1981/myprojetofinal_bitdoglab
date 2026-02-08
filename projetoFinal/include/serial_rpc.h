#ifndef SERIAL_RPC_H
#define SERIAL_RPC_H

/**
 * @file serial_rpc.h
 * @brief Task FreeRTOS para controle/telemetria via USB-Serial (JSON line-based).
 */

#include "FreeRTOS.h"
#include "task.h"

#ifdef __cplusplus
extern "C" {
#endif

struct app_ctx; // forward

/**
 * @brief Task do Serial RPC. Espera pvParameters = (struct app_ctx*).
 */
void vTaskSerialRpc(void *pvParameters);

#ifdef __cplusplus
}
#endif

#endif // SERIAL_RPC_H
