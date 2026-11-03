#include "controller_link.h"
#include "usart.h"
#include <string.h>

/* Latest input bytes and the bytes HAL is currently transmitting. */
static uint8_t latest_packet[CONTROLLER_PACKET_SIZE];
static uint8_t tx_packet[CONTROLLER_PACKET_SIZE];

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

void Controller_SetInputs(uint8_t buttons, const Joystick_State *joystick)
{
    uint8_t packet[CONTROLLER_PACKET_SIZE];
    packet[0] = buttons;
    for (unsigned i = 0; i < JOYSTICK_AXIS_COUNT; ++i) {
        float axis = joystick->valid ? joystick->normalized[i] : 0.0f;
        int16_t value = (int16_t)(axis * 1000.0f + (axis < 0.0f ? -0.5f : 0.5f));
        uint16_t wire = (uint16_t)value;
        packet[1U + 2U * i] = (uint8_t)wire;
        packet[2U + 2U * i] = (uint8_t)(wire >> 8);
    }
    packet[9] = Controller_Crc8(packet, 9U);

    /* Only this ten-byte copy needs protection from the TX callback. */
    uint32_t interrupt_mask = __get_PRIMASK();
    __disable_irq();
    memcpy(latest_packet, packet, sizeof(packet));
    __set_PRIMASK(interrupt_mask);
}

bool Controller_Transmit(void)
{
    /* Called only at startup or after the previous transfer has completed. */
    memcpy(tx_packet, latest_packet, sizeof(tx_packet));
    return HAL_UART_Transmit_IT(&huart1, tx_packet, sizeof(tx_packet)) == HAL_OK;
}

void Controller_GetPacket(uint8_t packet[CONTROLLER_PACKET_SIZE])
{
    uint32_t interrupt_mask = __get_PRIMASK();
    __disable_irq();
    memcpy(packet, tx_packet, sizeof(tx_packet));
    __set_PRIMASK(interrupt_mask);
}
