#include "ir_app.h"
#include "ir_config.h"
#include "ir_sensor.h"
#include "main.h"
#include "lcd/lcd.h"

static uint32_t last_update; 

void IR_Init(void)
{
    last_update = HAL_GetTick() - SENSOR_UPDATE_MS;
}


static void DrawIR(void)
{
    tft_prints(0, 3, "UART2 A0:R D0:T");
    if (!IR_WIRING_CONFIRMED) {
        tft_prints(0, 1, "IR WIRING CHECK");
        tft_prints(0, 2, "SURFACE: ---");
        return;
    }
    uint8_t d0 = 0U;
    if (IR_ReadDigital(&d0) != HAL_OK || d0 > 1U) {
        tft_prints(0, 1, "IR NOT READY");
        tft_prints(0, 2, "CHECK CW2 GPIO");
        return;
    }
    IR_Sample sample = {0};
    if (IR_Read(&sample) != HAL_OK || sample.a0 > 4095U || sample.d0 > 1U) {
        tft_prints(0, 1, "A0:---- D0:%u", (unsigned)d0);
        tft_prints(0, 2, "ADC TODO/ERROR");
        return;
    }
    tft_prints(0, 1, "A0:%4u D0:%u", (unsigned)sample.a0, (unsigned)sample.d0);
    ProcessIR(&sample);
}

void IR_Update(void)
{
    uint32_t now = HAL_GetTick();
    if ((uint32_t)(now - last_update) < SENSOR_UPDATE_MS) return;
    last_update = now;

    tft_prints(0, 0, "IR");

    DrawIR();

    tft_update(0);
}