#include "buttons.h"
#include "main.h"
#include <assert.h>
#include <stdio.h>

GPIO_TypeDef test_port_a = {0}, test_port_b = {1};
static uint16_t gpio_a = UINT16_MAX, gpio_b = UINT16_MAX;

GPIO_PinState HAL_GPIO_ReadPin(GPIO_TypeDef *port, uint16_t pin)
{
    return ((port == &test_port_a ? gpio_a : gpio_b) & pin)
        ? GPIO_PIN_SET : GPIO_PIN_RESET;
}

static void SetButtons(unsigned mask)
{
    /* Expected electrical map, independent of the production reader's table. */
    gpio_a = (uint16_t)(UINT16_MAX & ~(
        ((mask & 1U) ? (1U << 9) : 0U) |
        ((mask & 2U) ? (1U << 8) : 0U) |
        ((mask & 128U) ? (1U << 15) : 0U)));
    gpio_b = (uint16_t)(UINT16_MAX & ~(
        ((mask & 4U) ? (1U << 15) : 0U) |
        ((mask & 8U) ? (1U << 14) : 0U) |
        ((mask & 16U) ? (1U << 5) : 0U) |
        ((mask & 32U) ? (1U << 4) : 0U) |
        ((mask & 64U) ? (1U << 3) : 0U)));
}

static void TestButtons(void)
{
    Buttons_State s;
    for (unsigned mask = 0; mask < 256U; ++mask) {
        SetButtons(0);
        Buttons_Init(0);
        SetButtons(mask);
        Buttons_Read(1, &s);
        assert(s.down == 0 && s.pressed == 0);
        Buttons_Read(20, &s);
        assert(s.down == 0);
        Buttons_Read(21, &s);
        assert(s.down == mask && s.pressed == mask);
        Buttons_Read(71, &s);
        assert(s.pressed == 0 && s.released == 0);
        for (unsigned i = 0; i < 8; ++i)
            assert(s.held_ms[i] == ((mask & (1U << i)) ? 50U : 0U));
        SetButtons(0);
        Buttons_Read(80, &s);
        Buttons_Read(100, &s);
        assert(s.down == 0 && s.released == mask);
        for (unsigned i = 0; i < 8; ++i)
            assert(s.last_hold_ms[i] == ((mask & (1U << i)) ? 79U : 0U));
    }

    /* Chatter restarts the stability window; no spurious presses. */
    SetButtons(0); Buttons_Init(0);
    SetButtons(1); Buttons_Read(1, &s);
    SetButtons(0); Buttons_Read(10, &s);
    SetButtons(1); Buttons_Read(15, &s);
    Buttons_Read(34, &s); assert(s.down == 0);
    Buttons_Read(35, &s); assert(s.pressed == 1);

    /* Millisecond counter rollover and a button held at boot. */
    SetButtons(255); Buttons_Init(UINT32_MAX - 10U);
    Buttons_Read(8U, &s); assert(s.down == 0);
    Buttons_Read(9U, &s); assert(s.down == 255 && s.pressed == 255);
    Buttons_Read(109U, &s); assert(s.held_ms[7] == 100);
    Buttons_Read(110U, NULL);
}

int main(void)
{
    TestButtons();
    puts("PASS: eight-button mapping, debounce, edges, durations and timer rollover");
    return 0;
}
