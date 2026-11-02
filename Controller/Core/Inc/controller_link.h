#ifndef CONTROLLER_LINK_H
#define CONTROLLER_LINK_H

#include "joystick.h"
#include <stddef.h>

#define CONTROLLER_PACKET_SIZE 15U
#define CONTROLLER_TX_PERIOD_MS 20U
#define CONTROLLER_TX_TIMEOUT_MS 100U
#define CONTROLLER_FLAG_JOYSTICK_VALID 0x01U

/* CRC-8/SMBUS: polynomial 0x07, initial 0, no reflection, final XOR 0. */
uint8_t Controller_Crc8(const uint8_t *data, size_t length);
/* Initialize once before transmitting. USART1 is the existing HC-05 UART. */
void Controller_LinkInit(uint32_t now);
/* Returns immediately after starting TX, or false if not due/ready. */
bool Controller_TryTransmit(uint32_t now, uint8_t buttons, const Joystick_State *joystick);
/* Forward only USART1's HAL callbacks to these functions. */
void Controller_TransmitComplete(void);
void Controller_TransmitError(void);

#endif
