#include <stdio.h>

#include "FreeRTOS.h"
#include "task.h"

/**
 * @brief Hook do FreeRTOS chamado em stack overflow.
 * @note Implementação padrão: imprime e trava o sistema.
 */
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    (void)xTask;
    printf("\n*** STACK OVERFLOW: %s ***\n", pcTaskName ? pcTaskName : "(null)");
    taskDISABLE_INTERRUPTS();
    while (1) { }
}

/**
 * @brief Hook do FreeRTOS chamado quando malloc falha.
 * @note Implementação padrão: imprime e trava o sistema.
 */
void vApplicationMallocFailedHook(void)
{
    printf("\n*** MALLOC FAILED (heap) ***\n");
    taskDISABLE_INTERRUPTS();
    while (1) { }
}
