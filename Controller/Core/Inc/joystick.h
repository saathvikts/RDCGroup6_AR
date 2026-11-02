#ifndef TUTORIAL_JOYSTICK_H
#define TUTORIAL_JOYSTICK_H
#include <stdbool.h>
#include <stdint.h>
#include "joystick_config.h"

typedef struct {
    uint16_t raw[JOYSTICK_AXIS_COUNT];
    float normalized[JOYSTICK_AXIS_COUNT];
    bool valid;
} Joystick_State;
/* Called once, after the generated peripheral initialization. */
bool Joystick_Start(void);
/* On failure, return false and leave *raw unchanged. */
bool Joystick_ReadChannel(uint32_t channel, uint16_t *raw);
/* Read J1X/J1Y/J2X/J2Y. Failure clears the snapshot and its validity flag. */
bool Joystick_Read(Joystick_State *out);
#endif
