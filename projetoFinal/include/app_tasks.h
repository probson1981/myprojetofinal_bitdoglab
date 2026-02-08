#ifndef APP_TASKS_H
#define APP_TASKS_H

/**
 * @file app_tasks.h
 * @brief Tasks FreeRTOS da aplicação.
 */

#include "FreeRTOS.h"
#include "task.h"

#ifdef __cplusplus
extern "C" {
#endif

void vTaskLuminos(void *pvParameters);
void vTaskTempUmidade(void *pvParameters);
void vTaskDisplay(void *pvParameters);
void vTaskMqtt(void *pvParameters);

#ifdef __cplusplus
}
#endif

#endif // APP_TASKS_H
