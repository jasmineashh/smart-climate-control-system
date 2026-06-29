#ifndef INC_MCP2515_H_
#define INC_MCP2515_H_

#include "main.h"
#include "spi.h"
#include <stdint.h>

#define MCP2515_CS_LOW()   HAL_GPIO_WritePin(MCP2515_CS_GPIO_Port, MCP2515_CS_Pin, GPIO_PIN_RESET)
#define MCP2515_CS_HIGH()  HAL_GPIO_WritePin(MCP2515_CS_GPIO_Port, MCP2515_CS_Pin, GPIO_PIN_SET)

void MCP2515_Reset(void);
uint8_t MCP2515_Read(uint8_t address);
void MCP2515_Write(uint8_t address, uint8_t data);
void MCP2515_BitModify(uint8_t address, uint8_t mask, uint8_t data);
void MCP2515_Init(void);
uint8_t MCP2515_SendMessage(uint16_t id, uint8_t *data, uint8_t len);

#endif
