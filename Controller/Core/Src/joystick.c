#include "joystick.h"
#include "joystick_config.h"
#include "main.h"
#include "adc.h"

bool Joystick_Start(void)
{

    if(HAL_ADCEx_Calibration_Start(&hadc1)== HAL_OK){
        return true;
    }
    return false;
}

bool Joystick_ReadChannel(uint32_t channel, uint16_t *raw)
{
    /* Obtain one completed conversion from the requested channel.
       Use polling with a finite timeout. Report failure separately from raw 0.
       Keep the output unchanged unless the complete operation succeeds. */
    ADC_ChannelConfTypeDef config = {0};
    uint16_t value;

    if (raw == NULL) {
        return false;
    }

    config.Channel = channel;
    config.Rank = ADC_REGULAR_RANK_1;
    config.SamplingTime = ADC_SAMPLETIME_71CYCLES_5;
    if (HAL_ADC_ConfigChannel(&hadc1, &config) != HAL_OK) {
        return false;
    }
    if (HAL_ADC_Start(&hadc1) != HAL_OK) {
        HAL_ADC_Stop(&hadc1);
        return false;
    }
    if (HAL_ADC_PollForConversion(&hadc1, 10U) != HAL_OK) {
        HAL_ADC_Stop(&hadc1);
        return false;
    }

    value = (uint16_t)HAL_ADC_GetValue(&hadc1);
    if (HAL_ADC_Stop(&hadc1) != HAL_OK) {
        return false;
    }

    *raw = value;
    return true;

}
