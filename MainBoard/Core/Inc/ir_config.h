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