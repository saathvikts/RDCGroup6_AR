/* Host-test stand-ins for hardware. Production uses Core/Inc/main.h. */
#ifndef TEST_MAIN_H
#define TEST_MAIN_H
#include <stdint.h>
typedef struct { unsigned id; } GPIO_TypeDef;
typedef enum { GPIO_PIN_RESET = 0, GPIO_PIN_SET = 1 } GPIO_PinState;
extern GPIO_TypeDef test_port_a, test_port_b;
#define Button1_GPIO_Port (&test_port_a)
#define Button1_Pin (1U << 9)
#define Button2_GPIO_Port (&test_port_a)
#define Button2_Pin (1U << 8)
#define Button3_GPIO_Port (&test_port_b)
#define Button3_Pin (1U << 15)
#define Button4_GPIO_Port (&test_port_b)
#define Button4_Pin (1U << 14)
#define Button5_GPIO_Port (&test_port_b)
#define Button5_Pin (1U << 5)
#define Button6_GPIO_Port (&test_port_b)
#define Button6_Pin (1U << 4)
#define Button7_GPIO_Port (&test_port_b)
#define Button7_Pin (1U << 3)
#define Button8_GPIO_Port (&test_port_a)
#define Button8_Pin (1U << 15)
GPIO_PinState HAL_GPIO_ReadPin(GPIO_TypeDef *port, uint16_t pin);
#endif
