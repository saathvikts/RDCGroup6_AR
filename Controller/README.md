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

After GPIO/ADC/UART startup, main.c reads the initial inputs, publishes them and
calls Controller_StartTransmit() once, before while (1). This starts the first
15-byte transfer using HAL_UART_Transmit_IT(). HAL_UART_TxCpltCallback() forwards
USART1 completion to Controller_TransmitComplete(), which starts the next frame.
The STM32 HAL marks UART ready before calling this callback.

The main loop calls Buttons_Read() frequently and Joystick_Read() about every
20 ms. It calls Controller_PublishInputs() after a joystick sample or button
state change; this publishes input bytes without starting UART. Two alternating
snapshot buffers and a memory barrier keep partially updated inputs out of the
callback. A separate static TX buffer remains unchanged throughout each transfer.
Callbacks only copy prepared bytes, update the sequence/CRC, and start UART;
floating-point conversion, normalization and ADC polling stay in the main loop.

Frames stream continuously with no 20 ms transmission delay. At the configured
115200 baud, 8N1, a 15-byte frame takes about 1.3 ms on UART; unchanged input
samples therefore appear in multiple frames. The actual HC-05 radio throughput
and receiver still need checking on hardware.

HAL_UART_ErrorCallback() requests an interrupt-mode transmit abort, and the
abort-complete callback starts the next frame. A failed start/restart/abort stops
streaming and causes main.c to enter Error_Handler(), requiring reset. The chain
also stops if the latest published inputs reach 100 ms old, so an unresponsive
input loop cannot keep transmitting stale movement requests. The mainboard must
still implement its own receive timeout and zero drive requests on connection
loss. UART completion alone does not prove Bluetooth delivery.

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

### Build and hardware checks

Build firmware with `cmake --build --preset Debug` from Controller. Hardware
calibration, ST-Link flashing, UART/radio throughput and end-to-end mainboard
reception still need checking on the actual boards.

## Tutorial resources

- [Docs, warning, and usage on the Controller PCB](https://github.com/UST-Robotics-Team/Software-Tutorial-2026-Notes/blob/main/additional-notes/RDC-controller.md)
- [Tutorial 2 -- UART, Bluetooth](https://github.com/UST-Robotics-Team/Software-Tutorial-2026-Notes/tree/main/tutorial-2-uart-bluetooth)
- [Advanced Tutorial 1 -- ADC](https://github.com/UST-Robotics-Team/Software-Tutorial-2026-Notes/blob/main/advanced-tutorial-1-Advanced-Embedded-Systems/notes/01-adc.md)
- [Advanced Tutorial 1 -- Joystick example](https://github.com/UST-Robotics-Team/Software-Tutorial-2026-Notes/blob/main/advanced-tutorial-1-Advanced-Embedded-Systems/notes/02-joystick.md)
- Google is your friend! Do your own research and implement the code yourself.
