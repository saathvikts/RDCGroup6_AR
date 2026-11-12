#ifndef IR_SENSOR_H
#define IR_SENSOR_H

#include "main.h"

typedef struct { uint16_t a0; uint8_t d0; } IR_Sample;

HAL_StatusTypeDef IR_ReadDigital(uint8_t *d0);
HAL_StatusTypeDef IR_Read(IR_Sample *sample);
void ProcessIR(const IR_Sample *sample);

#endif
