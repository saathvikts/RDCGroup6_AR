#include "controller_receiver.h"
#include "usart.h"
#include "lcd/lcd.h"
#include <string.h>

typedef enum
{
  RECEIVER_WAIT_HEADER_0,
  RECEIVER_WAIT_HEADER_1,
  RECEIVER_READ_PACKET
} ControllerReceiver_State;

static UART_HandleTypeDef *receiver_uart;
static uint8_t receiver_byte;
static uint8_t receiver_packet[CONTROLLER_RECEIVER_PACKET_SIZE];
static uint8_t receiver_packet_index;
static volatile uint8_t receiver_buttons;
static volatile int16_t receiver_axes[4];
static volatile uint8_t receiver_buttons_pending;
static uint8_t receiver_previous_buttons;
static volatile bool receiver_packet_valid;
/* HAL_GetTick() of the last packet that passed the CRC check. */
static volatile uint32_t receiver_last_packet_tick;
static volatile uint16_t receiver_byte_count;
static volatile uint16_t receiver_packet_count;
static volatile uint16_t receiver_crc_error_count;
static volatile uint16_t receiver_overrun_count;
static volatile uint16_t receiver_frame_error_count;
static volatile uint16_t receiver_noise_error_count;
static ControllerReceiver_State receiver_state = RECEIVER_WAIT_HEADER_0;

static uint8_t ControllerReceiver_Crc8(const uint8_t *data, uint32_t length)
{
  uint8_t crc = 0U;
  for (uint32_t i = 0U; i < length; ++i)
  {
    crc ^= data[i];
    for (unsigned bit = 0U; bit < 8U; ++bit)
    {
      crc = (crc & 0x80U) != 0U
              ? (uint8_t)((crc << 1) ^ 0x07U)
              : (uint8_t)(crc << 1);
    }
  }
  return crc;
}

static void ControllerReceiver_HandlePacket(void)
{
  if (ControllerReceiver_Crc8(receiver_packet,
                              CONTROLLER_RECEIVER_CRC_OFFSET) !=
      receiver_packet[CONTROLLER_RECEIVER_CRC_OFFSET])
  {
    receiver_crc_error_count++;
    receiver_buttons_pending = 1U;
    receiver_state = RECEIVER_WAIT_HEADER_0;
    return;
  }

  for (unsigned axis = 0U; axis < 4U; ++axis)
  {
    unsigned offset = CONTROLLER_RECEIVER_AXES_OFFSET + 2U * axis;
    uint16_t wire = (uint16_t)receiver_packet[offset] |
                    ((uint16_t)receiver_packet[offset + 1U] << 8);
    receiver_axes[axis] = (int16_t)wire;
  }
  receiver_buttons = receiver_packet[CONTROLLER_RECEIVER_BUTTONS_OFFSET];
  receiver_buttons_pending = 1U;
  receiver_packet_valid = true;
  /* Only a packet that passed the CRC counts as "connected". */
  receiver_last_packet_tick = HAL_GetTick();
  receiver_packet_count++;
  receiver_state = RECEIVER_WAIT_HEADER_0;
}

HAL_StatusTypeDef ControllerReceiver_Start(UART_HandleTypeDef *huart)
{
  if (huart == NULL)
  {
    return HAL_ERROR;
  }

  receiver_uart = huart;
  receiver_state = RECEIVER_WAIT_HEADER_0;
  receiver_packet_index = 0U;
  receiver_buttons = 0U;
  for (unsigned axis = 0U; axis < 4U; ++axis)
  {
    receiver_axes[axis] = 0;
  }
  receiver_buttons_pending = 1U;
  receiver_previous_buttons = 0U;
  receiver_packet_valid = false;
  receiver_last_packet_tick = 0U;
  receiver_byte_count = 0U;
  receiver_packet_count = 0U;
  receiver_crc_error_count = 0U;
  receiver_overrun_count = 0U;
  receiver_frame_error_count = 0U;
  receiver_noise_error_count = 0U;

  return HAL_UART_Receive_IT(receiver_uart, &receiver_byte, 1U);
}

void ControllerReceiver_RxComplete(UART_HandleTypeDef *huart)
{
  if (huart != receiver_uart)
  {
    return;
  }

  receiver_byte_count++;

  switch (receiver_state)
  {
    case RECEIVER_WAIT_HEADER_0:
      if (receiver_byte == CONTROLLER_RECEIVER_HEADER_0)
      {
        receiver_packet[0] = receiver_byte;
        receiver_state = RECEIVER_WAIT_HEADER_1;
      }
      break;

    case RECEIVER_WAIT_HEADER_1:
      if (receiver_byte == CONTROLLER_RECEIVER_HEADER_1)
      {
        receiver_packet[1] = receiver_byte;
        receiver_packet_index = 2U;
        receiver_state = RECEIVER_READ_PACKET;
      }
      else if (receiver_byte == CONTROLLER_RECEIVER_HEADER_0)
      {
        receiver_packet[0] = receiver_byte;
      }
      else
      {
        receiver_state = RECEIVER_WAIT_HEADER_0;
      }
      break;

    case RECEIVER_READ_PACKET:
      receiver_packet[receiver_packet_index++] = receiver_byte;
      if (receiver_packet_index == CONTROLLER_RECEIVER_PACKET_SIZE)
      {
        ControllerReceiver_HandlePacket();
      }
      break;

    default:
      receiver_state = RECEIVER_WAIT_HEADER_0;
      break;
  }

  /*
   * Re-arm before returning so reception remains continuous. Errors are
   * recovered by HAL_UART_ErrorCallback below.
   */
  (void)HAL_UART_Receive_IT(receiver_uart, &receiver_byte, 1U);
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  ControllerReceiver_RxComplete(huart);
}

/*
 * Call at the top of USART1_IRQHandler(), before HAL_UART_IRQHandler().
 * huart->ErrorCode cannot be used for counting: re-arming reception inside
 * the Rx callback clears it before HAL_UART_ErrorCallback() runs. Reading
 * SR alone does not clear any flag.
 */
void ControllerReceiver_IrqHook(void)
{
  if (receiver_uart == NULL)
  {
    return;
  }

  uint32_t sr = receiver_uart->Instance->SR;
  if ((sr & USART_SR_RXNE) != 0U || (sr & USART_SR_ORE) != 0U)
  {
    if ((sr & USART_SR_ORE) != 0U) receiver_overrun_count++;
    if ((sr & USART_SR_FE) != 0U) receiver_frame_error_count++;
    if ((sr & USART_SR_NE) != 0U) receiver_noise_error_count++;
  }
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
  if (huart != receiver_uart)
  {
    return;
  }

  /*
   * HAL calls this callback AFTER the byte has already been delivered to
   * ControllerReceiver_RxComplete(), and reception is normally still armed.
   * Do not reset the parser here: the CRC rejects corrupted bytes. Only
   * re-arm if HAL actually stopped reception.
   */
  if (huart->RxState == HAL_UART_STATE_READY)
  {
    (void)HAL_UART_Receive_IT(receiver_uart, &receiver_byte, 1U);
  }
}

void ControllerReceiver_Draw(void)
{
  uint8_t buttons;
  int16_t axes[4];
  uint32_t interrupt_mask;
  char button_text[9];
  uint16_t packet_count, crc_error_count, uart_error_count;
  bool link_fresh = ControllerReceiver_IsFresh(CONTROLLER_RECEIVER_TIMEOUT_MS);

  /* Copy the ISR-updated values in one go so they belong together. */
  interrupt_mask = __get_PRIMASK();
  __disable_irq();
  buttons = receiver_buttons;
  packet_count = receiver_packet_count;
  crc_error_count = receiver_crc_error_count;
  uart_error_count = (uint16_t)(receiver_overrun_count +
                                receiver_frame_error_count +
                                receiver_noise_error_count);
  for (unsigned axis = 0U; axis < 4U; ++axis)
  {
    axes[axis] = receiver_axes[axis];
  }
  receiver_buttons_pending = 0U;
  __set_PRIMASK(interrupt_mask);

  for (unsigned i = 0U; i < 8U; ++i)
  {
    button_text[i] = (buttons & (1U << i)) != 0U ? '1' : '0';
  }
  button_text[8] = '\0';

  /* Rows 0-4 of the 10x16 TFT. The caller owns rows 5-9 and tft_update(). */
  tft_prints(0, 0, "PKT:%-4s R:%5u", link_fresh ? "OK" : "LOST",
             (unsigned)packet_count);
  tft_prints(0, 1, "B:%s", button_text);
  tft_prints(0, 2, "C:%5u E:%5u", (unsigned)crc_error_count,
             (unsigned)uart_error_count);
  /* tft_prints() has its own printf: no '+' flag, so use plain %5d. */
  tft_prints(0, 3, "J1 %5d %5d", (int)axes[0], (int)axes[1]);
  tft_prints(0, 4, "J2 %5d %5d", (int)axes[2], (int)axes[3]);
}

bool ControllerReceiver_IsPacketValid(void)
{
  return receiver_packet_valid;
}

bool ControllerReceiver_IsFresh(uint32_t timeout_ms)
{
  /* Read both ISR-updated values together so they belong to the same packet. */
  uint32_t interrupt_mask = __get_PRIMASK();
  __disable_irq();
  bool valid = receiver_packet_valid;
  uint32_t last_tick = receiver_last_packet_tick;
  __set_PRIMASK(interrupt_mask);

  /* Unsigned subtraction gives the packet's age, even across tick wrap. */
  return valid && (uint32_t)(HAL_GetTick() - last_tick) < timeout_ms;
}

uint8_t ControllerReceiver_GetButtons(void)
{
  return receiver_buttons;
}

int16_t ControllerReceiver_GetAxis(unsigned axis)
{
  if (axis >= 4U)
  {
    return 0;
  }

  return receiver_axes[axis];
}
