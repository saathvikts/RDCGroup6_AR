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

## Application and packet

### Controller application flow

After GPIO/ADC/UART startup, main.c initializes the button reader and transmitter.
The main loop calls Buttons_Read() frequently, Joystick_Read() about every 20 ms,
and Controller_TryTransmit() on each pass. The transmitter schedules at most one
attempt per 20 ms and uses HAL_UART_Transmit_IT(). Its static packet buffer stays
unchanged until the TX-complete or abort-complete callback. A UART error or a
100 ms local TX stall requests HAL_UART_AbortTransmit_IT() before the next frame.
This timeout monitors the local UART only; it does not establish Bluetooth link
health or prove that the mainboard received a frame.

ADC acquisition uses the existing bounded polling implementation (10 ms timeout
per conversion wait). UART transmission and its recovery are interrupt-based.
No HAL_Delay() is used in the application loop. The controller TFT is initialized
but no new diagnostic screen is drawn yet. MainBoard firmware is unchanged.

### Controller packet, version 1

Each frame is exactly 15 bytes. The eight button values occupy one byte; each
of the four normalized axes is rounded to a signed integer from -1000 to +1000.
For example, -1000 represents -1.0 and 500 represents +0.5. Axis integers use
16-bit two's-complement representation, low byte first. Serialize fields
explicitly; do not transmit a C struct's memory layout.

| Offset | Meaning |
| --- | --- |
| 0-1 | Start bytes A5 5A (hexadecimal) |
| 2 | Version = 1 |
| 3 | Sequence byte; increments on successful TX start, wraps 255 to 0 |
| 4 | Flags: bit 0 = all joystick axes valid; other bits zero |
| 5 | Buttons: bit 0 = Button1, ..., bit 7 = Button8; 1 = pressed |
| 6-7 | J1X, signed 16-bit, low byte first |
| 8-9 | J1Y, signed 16-bit, low byte first |
| 10-11 | J2X, signed 16-bit, low byte first |
| 12-13 | J2Y, signed 16-bit, low byte first |
| 14 | CRC-8 over bytes 0 through 13 |

CRC settings: polynomial 0x07, initial value 0x00, no input/output reflection,
final XOR 0x00 (CRC-8/SMBUS). The ASCII bytes `123456789` produce 0xF4.
Invalid ADC acquisition or an invalid normalized value clears flag bit 0 and
sends four zero axes; button states can still be sent.

The future receiver must buffer full frames, validate version/flags/axis ranges
and CRC, and track sequence freshness including wrap/restart. Header bytes may
also occur inside the payload: do not restart unconditionally whenever A5 5A
appears during a frame. After invalid framing/CRC, search again with a sliding
window. Only fresh accepted frames should refresh its communication timeout.
Mainboard drive requests must become zero on invalid joystick status or timeout;
neutral-stick recovery and suppression of synthetic button presses on reconnect
remain mainboard work.

These are snapshots of button states. Raw joystick counts and local button
edge/hold-time fields are not transmitted in version 1. The mainboard can
estimate holds from received transitions. Exact durations and guaranteed capture
of short taps across lost frames need a later protocol extension with timing or
press counters. No physical action has been assigned to a numbered button yet.

### Verification

Build firmware with `cmake --build --preset Debug` from Controller. Run
`python tests/run_host_tests.py` with host GCC installed to exercise the actual
input/transmitter C sources against fake GPIO, ADC and UART functions. Tests
cover all 256 button combinations, bounce, press/hold/release, timer rollover,
joystick endpoints/deadzones/signs, ADC failures, a known CRC vector, wire byte
order, in-flight buffer preservation, UART failures, abort recovery and sequence
rollover. These PC tests do not emulate hardware interrupts, sensor voltages,
ST-Link flashing or the HC-05 radio link. Hardware calibration and end-to-end
mainboard reception still need testing.

## Tutorial resources

- [Docs, warning, and usage on the Controller PCB](https://github.com/UST-Robotics-Team/Software-Tutorial-2026-Notes/blob/main/additional-notes/RDC-controller.md)
- [Tutorial 2 -- UART, Bluetooth](https://github.com/UST-Robotics-Team/Software-Tutorial-2026-Notes/tree/main/tutorial-2-uart-bluetooth)
- [Advanced Tutorial 1 -- ADC](https://github.com/UST-Robotics-Team/Software-Tutorial-2026-Notes/blob/main/advanced-tutorial-1-Advanced-Embedded-Systems/notes/01-adc.md)
- [Advanced Tutorial 1 -- Joystick example](https://github.com/UST-Robotics-Team/Software-Tutorial-2026-Notes/blob/main/advanced-tutorial-1-Advanced-Embedded-Systems/notes/02-joystick.md)
- Google is your friend! Do your own research and implement the code yourself.
