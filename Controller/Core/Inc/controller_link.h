#ifndef CONTROLLER_LINK_H
#define CONTROLLER_LINK_H

#include "joystick.h"
#include <stddef.h>

#define CONTROLLER_PACKET_SIZE 10U

/* CRC-8: polynomial 0x07, initial 0, no reflection, final XOR 0. */
uint8_t Controller_Crc8(const uint8_t *data, size_t length);
/* Prepare the next packet from the current inputs. Call in main, not the ISR. */
void Controller_SetInputs(uint8_t buttons, const Joystick_State *joystick);
/* Call once before while (1), then from HAL_UART_TxCpltCallback(). */
bool Controller_Transmit(void);
/* Copy a complete transmitted packet for the main-loop TFT display. */
void Controller_GetPacket(uint8_t packet[CONTROLLER_PACKET_SIZE]);

#endif
