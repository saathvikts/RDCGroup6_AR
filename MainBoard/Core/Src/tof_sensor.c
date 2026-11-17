#include "sensors.h"
#include "i2c.h"
#include "tof_config.h"
#include "vl53l1x_subset.h"
#include "vl53l1_platform.h"
static bool initialized;


HAL_StatusTypeDef TOF_Probe(void)
{
    return HAL_I2C_IsDeviceReady(&hi2c2, TOF_ADDRESS_HAL, 3, SENSOR_IO_TIMEOUT_MS);
}


HAL_StatusTypeDef TOF_Init(void)
{
    HAL_GPIO_WritePin(TOF_Shut_GPIO_Port, TOF_Shut_Pin, GPIO_PIN_RESET);
    HAL_Delay(100);
    initialized=false;
    VL53L1_PlatformBegin();
    HAL_StatusTypeDef probe = TOF_Probe();
    if (probe != HAL_OK) return probe;
    uint32_t start=HAL_GetTick();
    uint8_t booted=0;
    while (!booted) {
        if (VL53L1X_BootState(TOF_ADDRESS_HAL,&booted)) return HAL_ERROR;
        if ((uint32_t)(HAL_GetTick()-start)>=500U) return HAL_TIMEOUT;
        HAL_Delay(2);
    }
    uint16_t id=0;
    if (VL53L1X_GetSensorId(TOF_ADDRESS_HAL,&id) || id!=0xEACCU) return HAL_ERROR;
    int8_t s=VL53L1X_SensorInit(TOF_ADDRESS_HAL);
    if (s) return s==-2 ? HAL_TIMEOUT : HAL_ERROR;
    /* Normalize the timing budget first: the mirrored default table's encoded
       budget is not recognized by its getter. SetDistanceMode preserves it. */
    if (VL53L1X_SetTimingBudgetInMs(TOF_ADDRESS_HAL,50) ||
        VL53L1X_SetDistanceMode(TOF_ADDRESS_HAL,2) ||
        VL53L1X_SetInterMeasurementInMs(TOF_ADDRESS_HAL,100) ||
        VL53L1X_StartRanging(TOF_ADDRESS_HAL)) return HAL_ERROR;
    initialized=true;
    return HAL_OK;
}
HAL_StatusTypeDef TOF_Read(TOF_Sample *sample)
{
    if (!sample) return HAL_ERROR;
    sample->distance_mm = 0; sample->range_status = 255; sample->valid = false;
    if (!initialized) return HAL_ERROR;
    VL53L1_PlatformBegin();

    uint8_t data_ready = 0;
    uint32_t start_tick = HAL_GetTick();

    // 1. Wait for data ready with a 500 ms maximum timeout
    while (data_ready == 0) {
        if (VL53L1X_CheckForDataReady(TOF_ADDRESS_HAL, &data_ready) != 0) {
            sample->distance_mm = 0;
            initialized = false;
            return HAL_ERROR;
        }
        if ((HAL_GetTick() - start_tick) >= 500U) {
            sample->distance_mm = 0;
            initialized = false;
            return HAL_TIMEOUT;
        }
        HAL_Delay(2);
    }

    // 2. Read range status
    uint8_t range_status = 255;
    VL53L1X_GetRangeStatus(TOF_ADDRESS_HAL, &range_status);
    sample->range_status = range_status;

    // 3. Read distance (in mm)
    uint16_t distance = 0;
    VL53L1X_GetDistance(TOF_ADDRESS_HAL, &distance);
    sample->distance_mm = distance;

    // 4. Clear the sensor event so the next measurement can trigger
    if (VL53L1X_ClearInterrupt(TOF_ADDRESS_HAL) != 0) {
        sample->distance_mm = 0;
        initialized = false;
        return HAL_ERROR;
    }

    // 5. Evaluate validity: normalized status 0 and nonzero distance
    sample->valid = (sample->range_status == 0 && sample->distance_mm > 0);

    return HAL_OK;
}