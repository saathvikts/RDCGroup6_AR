#ifndef CONTROLLER_RECEIVER_H
#define CONTROLLER_RECEIVER_H

#include "stm32f4xx_hal.h"
#include <stdbool.h>
#include <stdint.h>

#define CONTROLLER_RECEIVER_PACKET_SIZE 12U
#define CONTROLLER_RECEIVER_HEADER_0 0xAAU
#define CONTROLLER_RECEIVER_HEADER_1 0x55U
#define CONTROLLER_RECEIVER_BUTTONS_OFFSET 2U
#define CONTROLLER_RECEIVER_AXES_OFFSET 3U
#define CONTROLLER_RECEIVER_CRC_OFFSET 11U

/*
 * Start interrupt-driven reception on the supplied UART.
 * The UART must have been initialized before this function is called.
 */
HAL_StatusTypeDef ControllerReceiver_Start(UART_HandleTypeDef *huart);

/*
 * Feed the receiver from HAL_UART_RxCpltCallback().
 * This function only parses and stores data; it does not run button commands.
 */
void ControllerReceiver_RxComplete(UART_HandleTypeDef *huart);

/*
 * Call at the top of USART1_IRQHandler(), before HAL_UART_IRQHandler(),
 * to count framing/noise/overrun errors from the raw status register.
 */
void ControllerReceiver_IrqHook(void);

/*
 * Run newly received button commands from the main loop.
 * A command is run once when its button changes from released to pressed.
 */
void ControllerReceiver_Process(void);

bool ControllerReceiver_IsPacketValid(void);
uint8_t ControllerReceiver_GetButtons(void);
int16_t ControllerReceiver_GetAxis(unsigned axis);

#endif
