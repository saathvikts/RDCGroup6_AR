#include "adc.h"
#include "main.h"
#include "sensors.h"
#include "lcd/lcd.h"


/* TODO: Implement functions in ir_sensor.h */
HAL_StatusTypeDef IR_ReadDigital(uint8_t *d0)
{
    if (d0 == NULL) {
        return HAL_ERROR;
    }       

    uint8_t d0_val = HAL_GPIO_ReadPin(IR_D0_UART2_T_GPIO_Port, IR_D0_UART2_T_Pin);

    if (d0_val == GPIO_PIN_SET) {
        *d0 = 1;
    } else {
        *d0 = 0;
    }
    return HAL_OK;
}

HAL_StatusTypeDef IR_Read(IR_Sample *sample)
{
    /* TODO CW2B: obtain A0 by single-conversion polling and D0 using your
       digital function. Check statuses, use a finite timeout and stop the ADC.
       Publish the sample only when the complete operation succeeds.
       F405 does not use the F103 ADC self-calibration call. */

    if (sample == NULL) {
        return HAL_ERROR;
    }
    
    IR_Sample value;
    HAL_StatusTypeDef status;

    if (HAL_ADC_Start(&hadc1) != HAL_OK) {
        HAL_ADC_Stop(&hadc1);
        return HAL_ERROR;
    }

    status = HAL_ADC_PollForConversion(&hadc1, 10);
    if (status == HAL_OK)
    {
        value.a0 = (uint16_t)HAL_ADC_GetValue(&hadc1);
        status = IR_ReadDigital(&value.d0);
    }

    HAL_ADC_Stop(&hadc1);

    // 5. Publish the sample only when the complete operation succeeds
    if (status == HAL_OK)
    {
        *sample = value;
        return HAL_OK;
    }

    return HAL_ERROR;
}

void ProcessIR(const IR_Sample *sample)
{
    /* TODO CW2C: classify successful A0 samples using your measured threshold and polarity. Draw the label on row 2; compare it with the separate D0. */
    if (sample == NULL)
    {
        return;
    }

    // Classify surface based on analog value (A0) and polarity configuration
#if !IR_BLACK_IS_HIGH
    // If black surface gives a higher analog value (> threshold)
    if (sample->a0 > IR_THRESHOLD)
    {
        tft_prints(0, 2, "SURFACE: BLACK");
    }
    else
    {
        tft_prints(0, 2, "SURFACE: WHITE");
    }
#else
    // If black surface gives a lower analog value (< threshold)
    if (sample->a0 < IR_THRESHOLD)
    {
        tft_prints(0, 2, "SURFACE: BLACK");
    }
    else
    {
        tft_prints(0, 2, "SURFACE: WHITE");
    }
#endif
}