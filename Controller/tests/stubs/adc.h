#ifndef TEST_ADC_H
#define TEST_ADC_H
#include "hal_types.h"
#include <stddef.h>
#include <stdint.h>
typedef struct { unsigned unused; } ADC_HandleTypeDef;
typedef struct { uint32_t Channel, Rank, SamplingTime; } ADC_ChannelConfTypeDef;
extern ADC_HandleTypeDef hadc1;
#define ADC_CHANNEL_0 0U
#define ADC_CHANNEL_1 1U
#define ADC_CHANNEL_2 2U
#define ADC_CHANNEL_3 3U
#define ADC_REGULAR_RANK_1 1U
#define ADC_SAMPLETIME_71CYCLES_5 6U
HAL_StatusTypeDef HAL_ADCEx_Calibration_Start(ADC_HandleTypeDef *adc);
HAL_StatusTypeDef HAL_ADC_ConfigChannel(ADC_HandleTypeDef *adc, ADC_ChannelConfTypeDef *config);
HAL_StatusTypeDef HAL_ADC_Start(ADC_HandleTypeDef *adc);
HAL_StatusTypeDef HAL_ADC_PollForConversion(ADC_HandleTypeDef *adc, uint32_t timeout);
uint32_t HAL_ADC_GetValue(ADC_HandleTypeDef *adc);
HAL_StatusTypeDef HAL_ADC_Stop(ADC_HandleTypeDef *adc);
#endif
