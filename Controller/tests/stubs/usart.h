#ifndef TEST_USART_H
#define TEST_USART_H
#include "hal_types.h"
#include <stdint.h>
typedef struct { unsigned unused; } UART_HandleTypeDef;
extern UART_HandleTypeDef huart1;
HAL_StatusTypeDef HAL_UART_Transmit_IT(UART_HandleTypeDef *uart, uint8_t *data, uint16_t size);
HAL_StatusTypeDef HAL_UART_AbortTransmit_IT(UART_HandleTypeDef *uart);
#endif
