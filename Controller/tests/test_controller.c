#include "buttons.h"
#include "main.h"
#include "joystick.h"
#include "adc.h"
#include "controller_link.h"
#include "usart.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

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

UART_HandleTypeDef huart1;
static uint8_t *uart_data;
static uint16_t uart_size;
static unsigned uart_calls, abort_calls;
static HAL_StatusTypeDef uart_status = HAL_OK, abort_status = HAL_OK;
static int defer_abort;

HAL_StatusTypeDef HAL_UART_Transmit_IT(UART_HandleTypeDef *uart, uint8_t *data, uint16_t size)
{
    assert(uart == &huart1);
    ++uart_calls;
    uart_data = data;
    uart_size = size;
    return uart_status;
}
HAL_StatusTypeDef HAL_UART_AbortTransmit_IT(UART_HandleTypeDef *uart)
{
    assert(uart == &huart1);
    ++abort_calls;
    if (abort_status == HAL_OK && !defer_abort) Controller_TransmitComplete();
    return abort_status;
}

static void AssertNeutralPacket(void)
{
    assert(uart_data[4] == 0);
    for (unsigned i = 6; i < 14; ++i) assert(uart_data[i] == 0);
}

static void TestLink(void)
{
    const uint8_t check[] = "123456789";
    assert(Controller_Crc8(check, 9) == 0xF4U);
    assert(Controller_Crc8(NULL, 0) == 0U);

    Joystick_State s = {.valid = true, .normalized = {-1.0f, 1.0f, 0.5f, -0.5f}};
    Controller_LinkInit(100);
    assert(Controller_TryTransmit(100, 0x81, &s));
    assert(uart_calls == 1 && uart_size == 15);
    const uint8_t expected[] = {
        0xA5, 0x5A, 1, 0, 1, 0x81,
        0x18, 0xFC, 0xE8, 0x03, 0xF4, 0x01, 0x0C, 0xFE
    };
    assert(memcmp(uart_data, expected, sizeof(expected)) == 0);
    assert(uart_data[14] == Controller_Crc8(uart_data, 14));

    uint8_t saved[CONTROLLER_PACKET_SIZE];
    memcpy(saved, uart_data, sizeof(saved));
    s.normalized[0] = 0.25f;
    assert(!Controller_TryTransmit(120, 0xFF, &s));
    assert(uart_calls == 1 && memcmp(uart_data, saved, sizeof(saved)) == 0);

    /* Every single-bit data error in this packet changes its CRC. */
    for (unsigned i = 0; i < 14; ++i) {
        for (unsigned bit = 0; bit < 8; ++bit) {
            uint8_t damaged[15];
            memcpy(damaged, saved, sizeof(damaged));
            damaged[i] ^= (uint8_t)(1U << bit);
            assert(Controller_Crc8(damaged, 14) != damaged[14]);
        }
    }

    Controller_TransmitComplete();
    assert(Controller_TryTransmit(120, 0xFF, &s));
    assert(uart_data[3] == 1 && uart_data[5] == 0xFF);
    assert(uart_data[6] == 250 && uart_data[7] == 0);
    Controller_TransmitComplete();
    unsigned calls_before = uart_calls;
    assert(!Controller_TryTransmit(139, 0, &s));
    assert(uart_calls == calls_before);

    s.valid = false;
    assert(Controller_TryTransmit(140, 0x55, &s));
    AssertNeutralPacket();
    assert(uart_data[5] == 0x55);
    Controller_TransmitComplete();
    s.valid = true; s.normalized[2] = NAN;
    assert(Controller_TryTransmit(160, 0, &s));
    AssertNeutralPacket();
    Controller_TransmitComplete();
    s.normalized[2] = 2.0f;
    assert(Controller_TryTransmit(180, 0, &s));
    AssertNeutralPacket();
    Controller_TransmitComplete();

    /* HAL_BUSY does not wedge the sender or advance the sequence. */
    Controller_LinkInit(200);
    uart_status = HAL_BUSY;
    assert(!Controller_TryTransmit(200, 0, NULL));
    calls_before = uart_calls;
    uart_status = HAL_OK;
    assert(!Controller_TryTransmit(219, 0, NULL) && uart_calls == calls_before);
    assert(Controller_TryTransmit(220, 0, NULL));
    assert(uart_data[3] == 0);
    AssertNeutralPacket();

    /* No completion: abort asynchronously before reusing the buffer. */
    unsigned aborts_before = abort_calls;
    memcpy(saved, uart_data, sizeof(saved));
    assert(!Controller_TryTransmit(319, 3, &s));
    assert(abort_calls == aborts_before);
    defer_abort = 1;
    assert(!Controller_TryTransmit(320, 3, &s));
    assert(abort_calls == aborts_before + 1);
    assert(!Controller_TryTransmit(321, 3, &s));
    assert(abort_calls == aborts_before + 1);
    assert(memcmp(saved, uart_data, sizeof(saved)) == 0);
    Controller_TransmitComplete(); /* Delayed abort-complete callback. */
    assert(Controller_TryTransmit(322, 3, NULL));

    /* An error requests recovery in the main loop, without releasing TX early. */
    Controller_TransmitError();
    defer_abort = 0;
    assert(!Controller_TryTransmit(323, 4, NULL));
    assert(abort_calls == aborts_before + 2);
    assert(Controller_TryTransmit(342, 4, NULL));
    Controller_TransmitComplete();

    /* If abort fails, retain the buffer and retry recovery on the next call. */
    assert(Controller_TryTransmit(362, 0, NULL));
    abort_status = HAL_ERROR;
    assert(!Controller_TryTransmit(462, 0, NULL));
    abort_status = HAL_OK;
    assert(!Controller_TryTransmit(463, 0, NULL));
    assert(Controller_TryTransmit(464, 0, NULL));
    Controller_TransmitComplete();

    /* Period and sequence rollover. */
    Controller_LinkInit(UINT32_MAX - 5U);
    assert(Controller_TryTransmit(UINT32_MAX - 5U, 0, NULL));
    Controller_TransmitComplete();
    assert(!Controller_TryTransmit(13, 0, NULL));
    assert(Controller_TryTransmit(14, 0, NULL));
    Controller_TransmitComplete();
    Controller_LinkInit(0);
    for (unsigned i = 0; i < 258; ++i) {
        assert(Controller_TryTransmit(i * 20U, 0, NULL));
        assert(uart_data[3] == (uint8_t)i);
        Controller_TransmitComplete();
    }
}

int main(void)
{
    TestButtons();
    puts("PASS: eight-button mapping, debounce, edges, durations and timer rollover");
    TestJoystick();
    puts("PASS: joystick calibration, deadzone, normalization, directions and ADC failures");
    TestLink();
    puts("PASS: CRC, packet encoding, TX buffer lifetime, UART failures and rollover");
    return 0;
}
