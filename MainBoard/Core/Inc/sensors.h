#ifndef SENSORS_H
#define SENSORS_H
#include "main.h"
#include <stdbool.h>

typedef struct { uint16_t a0; uint8_t d0; } IR_Sample;
typedef struct { uint16_t distance_mm; uint8_t range_status; bool valid; } TOF_Sample;

HAL_StatusTypeDef TOF_Probe(void);
HAL_StatusTypeDef TOF_Init(void);
HAL_StatusTypeDef TOF_Read(TOF_Sample *sample);


HAL_StatusTypeDef IR_ReadDigital(uint8_t *d0);
HAL_StatusTypeDef IR_Read(IR_Sample *sample);
void ProcessIR(const IR_Sample *sample);

#ifndef IR_CONFIG_H
#define IR_CONFIG_H

#ifndef IR_WIRING_CONFIRMED
#define IR_WIRING_CONFIRMED 1
#endif

/* Change after recording black/white samples at the same sensor height. */
#define IR_THRESHOLD 2000U
#define IR_BLACK_IS_HIGH 1
#define SENSOR_UPDATE_MS 300U

#endif
#endif

