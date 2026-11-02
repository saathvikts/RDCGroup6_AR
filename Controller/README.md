# Controller Resources

## Current TR configuration

ADC1 is configured for the existing joystick connections: PA0/IN0 = J1X,
PA1/IN1 = J1Y, PA2/IN2 = J2X, and PA3/IN3 = J2Y. Confirm which physical
joystick is J1 before assigning left/right driving controls.

Conversions use software triggering, a single conversion at Rank 1, and
71.5 ADC-clock sampling cycles. Scan mode, continuous conversion, ADC DMA and
ADC interrupts are disabled. The reading code will select each channel in turn;
`Joystick_ReadChannel()` selects one channel and obtains its raw value with a
bounded polling wait. `Joystick_Start()` calibrates ADC1 once during startup;
startup enters `Error_Handler()` if calibration fails. The main loop requests
all four axes about every 20 ms. `Joystick_Read()` provides raw values and
normalized values in [-1, +1], applies the deadzone and Y directions once, and
clears all values plus its validity flag when acquisition fails.

`Core/Inc/joystick_config.h` holds the imported tutorial settings for four axes:
centres of 2000 counts, a 120-count deadzone, and reversed Y directions. X
endpoints remain defaults and Y endpoints/centres are approximate. These values
still need measured TR calibration. These settings now control normalization.

`Buttons_Read()` polls Button1 through Button8 using the existing active-low
convention, accepts a change after 20 ms of stability, and records state, press
and release edges, current hold durations and the last completed hold durations.
Bit 0 represents Button1 and bit 7 represents Button8. The joystick push inputs
and the two LimitSwitch-labelled inputs are not included in these eight buttons.

USART1 remains on PB6 TX / PB7 RX at 115200 baud with its interrupt enabled.
The connected HC-05 modules still need their actual settings and wiring checked.

Build from this Controller folder with `cmake --preset Debug`, followed by
`cmake --build --preset Debug`. The output is `build/Debug/Controller.elf`.

## Tutorial resources

- [Docs, warning, and usage on the Controller PCB](https://github.com/UST-Robotics-Team/Software-Tutorial-2026-Notes/blob/main/additional-notes/RDC-controller.md)
- [Tutorial 2 -- UART, Bluetooth](https://github.com/UST-Robotics-Team/Software-Tutorial-2026-Notes/tree/main/tutorial-2-uart-bluetooth)
- [Advanced Tutorial 1 -- ADC](https://github.com/UST-Robotics-Team/Software-Tutorial-2026-Notes/blob/main/advanced-tutorial-1-Advanced-Embedded-Systems/notes/01-adc.md)
- [Advanced Tutorial 1 -- Joystick example](https://github.com/UST-Robotics-Team/Software-Tutorial-2026-Notes/blob/main/advanced-tutorial-1-Advanced-Embedded-Systems/notes/02-joystick.md)
- Google is your friend! Do your own research and implement the code yourself.
