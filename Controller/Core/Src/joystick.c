#include "joystick.h"
#include "joystick_config.h"
#include "main.h"
#include "adc.h"

bool Joystick_Start(void)
{
    return HAL_ADCEx_Calibration_Start(&hadc1) == HAL_OK;
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

/* Reused from the completed CW1_NormalizeAxis() tutorial exercise. */
static float Normalize(uint16_t raw, uint16_t minimum, uint16_t centre,
                       uint16_t maximum, uint16_t deadband)
{
    int32_t offset = (int32_t)raw - (int32_t)centre;
    int32_t negative_span = (int32_t)centre - (int32_t)minimum;
    int32_t positive_span = (int32_t)maximum - (int32_t)centre;
    float value;

    if (offset > (int32_t)deadband) {
        value = (float)(offset - (int32_t)deadband) /
                (float)(positive_span - (int32_t)deadband);
    } else if (offset < -(int32_t)deadband) {
        value = (float)(offset + (int32_t)deadband) /
                (float)(negative_span - (int32_t)deadband);
    } else {
        return 0.0f;
    }
    if (value > 1.0f) return 1.0f;
    if (value < -1.0f) return -1.0f;
    return value;
}

bool Joystick_Read(Joystick_State *out)
{
    if (out == NULL) return false;

    const uint32_t channels[4] = {
        ADC_CHANNEL_0, ADC_CHANNEL_1, ADC_CHANNEL_2, ADC_CHANNEL_3
    };
    const uint16_t minimum[4] = {J1_X_MIN, J1_Y_MIN, J2_X_MIN, J2_Y_MIN};
    const uint16_t centre[4] = {J1_X_CENTRE, J1_Y_CENTRE, J2_X_CENTRE, J2_Y_CENTRE};
    const uint16_t maximum[4] = {J1_X_MAX, J1_Y_MAX, J2_X_MAX, J2_Y_MAX};
    const int direction[4] = {1, J1_Y_DIRECTION, 1, J2_Y_DIRECTION};
    Joystick_State next = {0};
    *out = next;

    for (unsigned i = 0; i < JOYSTICK_AXIS_COUNT; ++i) {
        /* Reject unusable calibration instead of dividing by zero. */
        if (minimum[i] >= centre[i] || centre[i] >= maximum[i] ||
            maximum[i] > 4095U ||
            (uint32_t)(centre[i] - minimum[i]) <= JOYSTICK_DEADBAND ||
            (uint32_t)(maximum[i] - centre[i]) <= JOYSTICK_DEADBAND ||
            (direction[i] != 1 && direction[i] != -1)) {
            return false;
        }
        if (!Joystick_ReadChannel(channels[i], &next.raw[i]) || next.raw[i] > 4095U) {
            return false;
        }
        next.normalized[i] = Normalize(next.raw[i], minimum[i], centre[i],
                                      maximum[i], JOYSTICK_DEADBAND) * direction[i];
    }
    next.valid = true;
    *out = next;
    return true;
}
