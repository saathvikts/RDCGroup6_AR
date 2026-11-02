#include "controller_link.h"
#include "usart.h"
#include <math.h>
#include <string.h>

/* HAL retains this buffer until TX completes or abort completes. */
static uint8_t tx_packet[CONTROLLER_PACKET_SIZE];
/* Main writes an inactive slot, then publishes its index atomically. The ISR
   reads the published slot and copies it into the separate TX buffer. */
static uint8_t prepared[2][CONTROLLER_PACKET_SIZE];
static uint32_t prepared_at[2];
static volatile uint32_t published_index;
static volatile bool tx_busy;
static volatile bool streaming;
static volatile bool recovering;
static volatile bool tx_failed;
static uint8_t sequence;

uint8_t Controller_Crc8(const uint8_t *data, size_t length)
{
    uint8_t crc = 0;
    for (size_t i = 0; i < length; ++i) {
        crc ^= data[i];
        for (unsigned bit = 0; bit < 8U; ++bit) {
            if ((crc & 0x80U) != 0U) {
                crc = (uint8_t)((crc << 1) ^ 0x07U);
            } else {
                crc = (uint8_t)(crc << 1);
            }
        }
    }
    return crc;
}

static void PrepareInputs(uint8_t *packet, uint8_t buttons, const Joystick_State *joystick)
{
    bool valid = joystick != NULL && joystick->valid;
    if (valid) {
        for (unsigned i = 0; i < JOYSTICK_AXIS_COUNT; ++i) {
            float axis = joystick->normalized[i];
            if (!isfinite(axis) || axis < -1.0f || axis > 1.0f) valid = false;
        }
    }

    packet[0] = 0xA5U;
    packet[1] = 0x5AU;
    packet[2] = 1U;
    packet[3] = 0U;  /* Sequence is filled when TX starts. */
    packet[4] = valid ? CONTROLLER_FLAG_JOYSTICK_VALID : 0U;
    packet[5] = buttons;
    for (unsigned i = 0; i < JOYSTICK_AXIS_COUNT; ++i) {
        float axis = valid ? joystick->normalized[i] : 0.0f;
        int16_t value = (int16_t)(axis * 1000.0f + (axis < 0.0f ? -0.5f : 0.5f));
        uint16_t wire = (uint16_t)value;
        packet[6U + 2U * i] = (uint8_t)wire;
        packet[7U + 2U * i] = (uint8_t)(wire >> 8);
    }
    packet[14] = 0U;  /* CRC includes the sequence, so fill it at TX start. */
}

void Controller_LinkInit(void)
{
    tx_busy = false;
    streaming = false;
    recovering = false;
    tx_failed = false;
    sequence = 0;
    PrepareInputs(prepared[0], 0U, NULL);
    prepared_at[0] = HAL_GetTick();
    __DMB();
    published_index = 0U;
}

void Controller_PublishInputs(uint8_t buttons, const Joystick_State *joystick)
{
    uint32_t next = published_index ^ 1U;
    PrepareInputs(prepared[next], buttons, joystick);
    prepared_at[next] = HAL_GetTick();
    /* Make all writes visible before the ISR can select this completed slot. */
    __DMB();
    published_index = next;
}

static bool StartNextPacket(void)
{
    uint32_t current = published_index;
    __DMB();
    /* A callback chain must not keep stale controls alive if main stops updating. */
    if ((uint32_t)(HAL_GetTick() - prepared_at[current]) >= CONTROLLER_INPUT_MAX_AGE_MS) {
        streaming = false;
        tx_failed = true;
        return false;
    }
    memcpy(tx_packet, prepared[current], sizeof(tx_packet));
    tx_packet[3] = sequence++;
    tx_packet[14] = Controller_Crc8(tx_packet, 14U);
    tx_failed = false;
    tx_busy = true;  /* Set before HAL can enable the interrupt. */
    if (HAL_UART_Transmit_IT(&huart1, tx_packet, CONTROLLER_PACKET_SIZE) != HAL_OK) {
        --sequence;
        tx_busy = false;
        streaming = false;
        tx_failed = true;
        return false;
    }
    return true;
}

bool Controller_StartTransmit(void)
{
    if (streaming || tx_busy) return false;
    streaming = true;
    return StartNextPacket();
}

bool Controller_TransmitFailed(void)
{
    return tx_failed;
}

void Controller_TransmitComplete(void)
{
    if (!streaming || recovering || !tx_busy) return;
    tx_busy = false;
    /* HAL has marked UART ready before invoking its TX-complete callback. */
    StartNextPacket();
}

void Controller_TransmitAbortComplete(void)
{
    if (!recovering) return;
    recovering = false;
    tx_busy = false;
    if (streaming) StartNextPacket();
}

void Controller_TransmitError(void)
{
    if (!streaming || recovering) return;
    recovering = true;
    /* Keep TX memory unchanged until abort completion. Callbacks perform no
       ADC acquisition, normalization, waits or unbounded retry loops. */
    if (HAL_UART_AbortTransmit_IT(&huart1) != HAL_OK) {
        streaming = false;
        tx_failed = true;
    }
}
