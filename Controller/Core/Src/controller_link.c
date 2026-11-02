#include "controller_link.h"
#include "usart.h"
#include <math.h>

/* HAL keeps this buffer's address until TX completes or abort completes. */
static uint8_t tx_packet[CONTROLLER_PACKET_SIZE];
static volatile bool tx_busy;
static volatile bool tx_aborting;
static volatile bool tx_error;
static uint8_t sequence;
static uint32_t last_attempt;
static uint32_t tx_started;

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

static void BuildPacket(uint8_t buttons, const Joystick_State *joystick)
{
    bool valid = joystick != NULL && joystick->valid;
    if (valid) {
        for (unsigned i = 0; i < JOYSTICK_AXIS_COUNT; ++i) {
            float axis = joystick->normalized[i];
            if (!isfinite(axis) || axis < -1.0f || axis > 1.0f) valid = false;
        }
    }

    tx_packet[0] = 0xA5U;
    tx_packet[1] = 0x5AU;
    tx_packet[2] = 1U;  /* Protocol version. */
    tx_packet[3] = sequence;
    tx_packet[4] = valid ? CONTROLLER_FLAG_JOYSTICK_VALID : 0U;
    tx_packet[5] = buttons;
    for (unsigned i = 0; i < JOYSTICK_AXIS_COUNT; ++i) {
        float axis = valid ? joystick->normalized[i] : 0.0f;
        int16_t value = (int16_t)(axis * 1000.0f + (axis < 0.0f ? -0.5f : 0.5f));
        uint16_t wire = (uint16_t)value;
        tx_packet[6U + 2U * i] = (uint8_t)wire;
        tx_packet[7U + 2U * i] = (uint8_t)(wire >> 8);
    }
    tx_packet[14] = Controller_Crc8(tx_packet, 14U);
}

void Controller_LinkInit(uint32_t now)
{
    tx_busy = false;
    tx_aborting = false;
    tx_error = false;
    sequence = 0;
    last_attempt = now - CONTROLLER_TX_PERIOD_MS;
    tx_started = now;
}

bool Controller_TryTransmit(uint32_t now, uint8_t buttons, const Joystick_State *joystick)
{
    if (tx_busy) {
        /* Recovery concerns the local UART, not Bluetooth acknowledgements. */
        if (!tx_aborting && (tx_error ||
            (uint32_t)(now - tx_started) >= CONTROLLER_TX_TIMEOUT_MS)) {
            tx_aborting = true;
            if (HAL_UART_AbortTransmit_IT(&huart1) != HAL_OK) {
                tx_aborting = false;
            }
        }
        return false;
    }
    if ((uint32_t)(now - last_attempt) < CONTROLLER_TX_PERIOD_MS) return false;

    last_attempt = now;
    BuildPacket(buttons, joystick);
    tx_started = now;
    tx_error = false;
    tx_busy = true;  /* Set before HAL can enable the interrupt. */
    if (HAL_UART_Transmit_IT(&huart1, tx_packet, CONTROLLER_PACKET_SIZE) != HAL_OK) {
        tx_busy = false;
        return false;
    }
    ++sequence;  /* Unsigned byte wraps from 255 to 0. */
    return true;
}

void Controller_TransmitComplete(void)
{
    tx_error = false;
    tx_aborting = false;
    tx_busy = false;
}

void Controller_TransmitError(void)
{
    /* Do not release or overwrite an in-flight buffer from an error callback. */
    if (tx_busy) tx_error = true;
}
