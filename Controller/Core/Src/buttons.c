#include "buttons.h"
#include "main.h"

static GPIO_TypeDef *const ports[BUTTON_COUNT] = {
    Button1_GPIO_Port, Button2_GPIO_Port, Button3_GPIO_Port, Button4_GPIO_Port,
    Button5_GPIO_Port, Button6_GPIO_Port, Button7_GPIO_Port, Button8_GPIO_Port
};
static const uint16_t pins[BUTTON_COUNT] = {
    Button1_Pin, Button2_Pin, Button3_Pin, Button4_Pin,
    Button5_Pin, Button6_Pin, Button7_Pin, Button8_Pin
};

uint8_t Buttons_Read(void)
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
