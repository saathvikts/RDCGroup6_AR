#include "buttons.h"
#include "main.h"
#include "joystick.h"
#include "adc.h"
#include <assert.h>
#include <math.h>
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

ADC_HandleTypeDef hadc1;
static unsigned adc_channel, adc_fail_channel;
static unsigned adc_fail_stage;
static uint16_t adc_values[4];
static HAL_StatusTypeDef calibration_status = HAL_OK;

HAL_StatusTypeDef HAL_ADCEx_Calibration_Start(ADC_HandleTypeDef *adc)
{
    assert(adc == &hadc1);
    return calibration_status;
}
HAL_StatusTypeDef HAL_ADC_ConfigChannel(ADC_HandleTypeDef *adc, ADC_ChannelConfTypeDef *c)
{
    assert(adc == &hadc1 && c->Channel < 4U);
    assert(c->Rank == ADC_REGULAR_RANK_1);
    assert(c->SamplingTime == ADC_SAMPLETIME_71CYCLES_5);
    adc_channel = c->Channel;
    return adc_fail_stage == 1 && adc_channel == adc_fail_channel ? HAL_ERROR : HAL_OK;
}
HAL_StatusTypeDef HAL_ADC_Start(ADC_HandleTypeDef *adc)
{
    assert(adc == &hadc1);
    return adc_fail_stage == 2 && adc_channel == adc_fail_channel ? HAL_ERROR : HAL_OK;
}
HAL_StatusTypeDef HAL_ADC_PollForConversion(ADC_HandleTypeDef *adc, uint32_t timeout)
{
    assert(adc == &hadc1 && timeout == 10U);
    return adc_fail_stage == 3 && adc_channel == adc_fail_channel ? HAL_TIMEOUT : HAL_OK;
}
uint32_t HAL_ADC_GetValue(ADC_HandleTypeDef *adc)
{
    assert(adc == &hadc1);
    return adc_values[adc_channel];
}
HAL_StatusTypeDef HAL_ADC_Stop(ADC_HandleTypeDef *adc)
{
    assert(adc == &hadc1);
    return adc_fail_stage == 4 && adc_channel == adc_fail_channel ? HAL_ERROR : HAL_OK;
}

static void TestJoystick(void)
{
    Joystick_State s;
    assert(Joystick_Start());
    calibration_status = HAL_ERROR;
    assert(!Joystick_Start());
    calibration_status = HAL_OK;

    for (unsigned i = 0; i < 4; ++i) adc_values[i] = 2000;
    assert(Joystick_Read(&s) && s.valid);
    for (unsigned i = 0; i < 4; ++i) {
        assert(s.raw[i] == 2000 && s.normalized[i] == 0.0f);
    }
    adc_values[0] = 1880; adc_values[1] = 2120;
    adc_values[2] = 2120; adc_values[3] = 1880;
    assert(Joystick_Read(&s));
    for (unsigned i = 0; i < 4; ++i) assert(s.normalized[i] == 0.0f);

    for (unsigned i = 0; i < 4; ++i) adc_values[i] = 0;
    assert(Joystick_Read(&s));
    assert(s.normalized[0] == -1.0f && s.normalized[1] == 1.0f);
    assert(s.normalized[2] == -1.0f && s.normalized[3] == 1.0f);
    adc_values[0] = 4095; adc_values[1] = 4000;
    adc_values[2] = 4095; adc_values[3] = 4095;
    assert(Joystick_Read(&s));
    assert(s.normalized[0] == 1.0f && s.normalized[1] == -1.0f);
    assert(s.normalized[2] == 1.0f && s.normalized[3] == -1.0f);
    adc_values[0] = 1000;
    assert(Joystick_Read(&s));
    assert(fabsf(s.normalized[0] - (-880.0f / 1880.0f)) < 0.00001f);

    /* Each ADC operation can fail on any axis; never publish partial/stale data. */
    for (adc_fail_stage = 1; adc_fail_stage <= 4; ++adc_fail_stage) {
        for (adc_fail_channel = 0; adc_fail_channel < 4; ++adc_fail_channel) {
            assert(!Joystick_Read(&s) && !s.valid);
            for (unsigned i = 0; i < 4; ++i)
                assert(s.raw[i] == 0 && s.normalized[i] == 0.0f);
        }
    }
    adc_fail_stage = 0;
    adc_values[2] = 5000;
    assert(!Joystick_Read(&s) && !s.valid);
    assert(!Joystick_Read(NULL));
}

int main(void)
{
    TestButtons();
    puts("PASS: eight-button mapping, debounce, edges, durations and timer rollover");
    TestJoystick();
    puts("PASS: joystick calibration, deadzone, normalization, directions and ADC failures");
    return 0;
}
