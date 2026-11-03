#ifndef CONTROLLER_BUTTONS_H
#define CONTROLLER_BUTTONS_H

#include <stdint.h>

#define BUTTON_COUNT 8U
/* Bit 0 = Button1, ..., bit 7 = Button8. A pressed button is represented by 1. */
uint8_t Buttons_Read(void);

#endif
