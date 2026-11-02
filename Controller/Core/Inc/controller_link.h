#ifndef CONTROLLER_LINK_H
#define CONTROLLER_LINK_H

#include "joystick.h"
#include <stddef.h>

#define CONTROLLER_PACKET_SIZE 15U
#define CONTROLLER_INPUT_MAX_AGE_MS 100U
#define CONTROLLER_FLAG_JOYSTICK_VALID 0x01U

/* CRC-8/SMBUS: polynomial 0x07, initial 0, no reflection, final XOR 0. */
uint8_t Controller_Crc8(const uint8_t *data, size_t length);
/* Initialize once before transmitting. USART1 is the existing HC-05 UART. */
void Controller_LinkInit(void);
/* Main loop publishes a complete snapshot; this does not start UART. */
void Controller_PublishInputs(uint8_t buttons, const Joystick_State *joystick);
/* Call once before while (1); later frames start from the TX callback. */
bool Controller_StartTransmit(void);
/* Forward only USART1's HAL callbacks to these functions. */
void Controller_TransmitComplete(void);
void Controller_TransmitAbortComplete(void);
void Controller_TransmitError(void);
/* True if starting/restarting TX failed; main.c enters Error_Handler(). */
bool Controller_TransmitFailed(void);

#endif
