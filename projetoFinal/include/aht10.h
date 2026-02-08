#ifndef AHT10_H
#define AHT10_H

#include <stdint.h>
#include <stdbool.h>
#include "hardware/i2c.h"

// Endereço I2C padrão do AHT10
#define AHT10_I2C_ADDRESS 0x38

// Comandos AHT10
#define AHT10_CMD_INITIALIZE 0xE1
#define AHT10_CMD_MEASURE    0xAC
#define AHT10_CMD_RESET      0xBA

// Estrutura para abstração do sensor
typedef struct {
    i2c_inst_t *i2c_port;  // <- contexto
    int (*i2c_write)(i2c_inst_t *port, uint8_t addr, const uint8_t *data, uint16_t len);
    int (*i2c_read)(i2c_inst_t *port, uint8_t addr, uint8_t *data, uint16_t len);
    void (*delay_ms)(uint32_t ms);
} AHT10_Interface;

typedef struct {
    AHT10_Interface iface;
    bool initialized;
} AHT10_Handle;

// Inicializa o sensor
bool AHT10_Init(AHT10_Handle *dev);

// Realiza medição e obtém temperatura (°C) e umidade (%)
bool AHT10_ReadTemperatureHumidity(AHT10_Handle *dev, float *temperature, float *humidity);

// Reinicializa o sensor
bool AHT10_SoftReset(AHT10_Handle *dev);

// Verifica se o sensor está ocupado (status busy)
bool AHT10_IsBusy(AHT10_Handle *dev);

// Prototipos das funções I2C
int i2c_write(i2c_inst_t *i2c_port, uint8_t addr, const uint8_t *data, uint16_t len);
int i2c_read(i2c_inst_t *i2c_port, uint8_t addr, uint8_t *data, uint16_t len);
void delay_ms(uint32_t ms);

#endif // AHT10_H
