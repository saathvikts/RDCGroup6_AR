#include "buttons.h"
#include "main.h"
#include <stddef.h>

static GPIO_TypeDef *const ports[BUTTON_COUNT] = {
    Button1_GPIO_Port, Button2_GPIO_Port, Button3_GPIO_Port, Button4_GPIO_Port,
    Button5_GPIO_Port, Button6_GPIO_Port, Button7_GPIO_Port, Button8_GPIO_Port
};
static const uint16_t pins[BUTTON_COUNT] = {
    Button1_Pin, Button2_Pin, Button3_Pin, Button4_Pin,
    Button5_Pin, Button6_Pin, Button7_Pin, Button8_Pin
};

static Buttons_State state;
static uint8_t candidate;
static uint32_t changed_at[BUTTON_COUNT];
static uint32_t pressed_at[BUTTON_COUNT];

static uint8_t ReadRaw(void)
{
    uint8_t mask = 0;
    for (unsigned i = 0; i < BUTTON_COUNT; ++i) {
        /* Existing controller convention: LOW means pressed. */
        if (HAL_GPIO_ReadPin(ports[i], pins[i]) == GPIO_PIN_RESET) {
            mask |= (uint8_t)(1U << i);
        }
    }
    return mask;
}

void Buttons_Init(uint32_t now)
{
    state = (Buttons_State){0};
    candidate = ReadRaw();
    for (unsigned i = 0; i < BUTTON_COUNT; ++i) {
        changed_at[i] = now;
        pressed_at[i] = now;
    }
}

void Buttons_Read(uint32_t now, Buttons_State *out)
{
    if (out == NULL) return;

    uint8_t raw = ReadRaw();
    state.pressed = 0;
    state.released = 0;

    for (unsigned i = 0; i < BUTTON_COUNT; ++i) {
        uint8_t bit = (uint8_t)(1U << i);
        if ((raw & bit) != (candidate & bit)) {
            candidate ^= bit;
            changed_at[i] = now;
        }

        if ((candidate & bit) != (state.down & bit) &&
            (uint32_t)(now - changed_at[i]) >= BUTTON_DEBOUNCE_MS) {
            state.down ^= bit;
            if ((state.down & bit) != 0U) {
                state.pressed |= bit;
                pressed_at[i] = now;
            } else {
                state.released |= bit;
                state.last_hold_ms[i] = (uint32_t)(now - pressed_at[i]);
            }
        }
        state.held_ms[i] = (state.down & bit) != 0U
            ? (uint32_t)(now - pressed_at[i]) : 0U;
    }
    *out = state;
}
