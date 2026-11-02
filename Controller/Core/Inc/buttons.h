#ifndef CONTROLLER_BUTTONS_H
#define CONTROLLER_BUTTONS_H

#include <stdint.h>

#define BUTTON_COUNT 8U
#define BUTTON_DEBOUNCE_MS 20U

typedef struct {
    uint8_t down;       /* Bit 0 = Button1, ..., bit 7 = Button8; 1 = pressed. */
    uint8_t pressed;    /* New debounced presses during this read. */
    uint8_t released;   /* New debounced releases during this read. */
    uint32_t held_ms[BUTTON_COUNT];
    uint32_t last_hold_ms[BUTTON_COUNT];
} Buttons_State;

/* Call after MX_GPIO_Init(), then call Buttons_Read() frequently. */
void Buttons_Init(uint32_t now);
void Buttons_Read(uint32_t now, Buttons_State *out);

#endif
