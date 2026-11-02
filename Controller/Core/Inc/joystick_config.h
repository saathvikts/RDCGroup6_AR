#ifndef JOYSTICK_CONFIG_H
#define JOYSTICK_CONFIG_H
/* Axis order: J1X, J1Y, J2X, J2Y (ADC1 channels 0, 1, 2, 3). */
#define JOYSTICK_AXIS_COUNT 4U
#if JOYSTICK_AXIS_COUNT != 4U
#error "The TR controller packet requires four joystick axes"
#endif
/* Imported tutorial values: approximate centres/Y stops; X endpoints are defaults.
   Replace these with measured calibration for the TR controller. */
#define J1_X_MIN 0U
#define J1_X_MAX 4095U
#define J1_X_CENTRE 2000U
#define J1_Y_MIN 0U
#define J1_Y_MAX 4000U
#define J1_Y_CENTRE 2000U
#define J2_X_MIN 0U
#define J2_X_MAX 4095U
#define J2_X_CENTRE 2000U
#define J2_Y_MIN 0U
#define J2_Y_MAX 4000U
#define J2_Y_CENTRE 2000U
/* Apply once after normalization: -1 reverses the ADC-positive direction. */
#define J1_Y_DIRECTION (-1)
#define J2_Y_DIRECTION (-1)
#define JOYSTICK_DEADBAND 120U
#endif
