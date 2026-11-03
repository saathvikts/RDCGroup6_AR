# Controller

This firmware reads eight buttons and four joystick axes, sends those values
with CRC-8 through USART1, and displays a transmitted packet on the controller TFT.

## Main loop

1. `Buttons_Read()` returns the eight raw button states in one byte.
2. `Joystick_Read()` reads J1X/J1Y/J2X/J2Y and normalizes them to [-1, +1].
3. `Controller_SetInputs()` packs the values and calculates CRC-8.
4. `DisplayInputs()` shows a transmitted packet, refreshing about every 100 ms.

Transmission starts once before `while (1)` using `Controller_Transmit()` and
`HAL_UART_Transmit_IT()`. `HAL_UART_TxCpltCallback()` starts the next packet after
the previous one finishes. The loop does not start transmissions. There is no
abort/restart handling, stale-input timer, button event/hold tracking, debounce,
sequence number, version field or status field.

The transmitter keeps two ten-byte arrays: the latest input packet and the
packet HAL is sending. Only the completed-transfer callback changes the send
array. A short protected copy prevents an interrupt from seeing half an update.
The TFT copies the send array so its displayed values and CRC belong together.
Only UART transmission is interrupt-driven; ADC and TFT use their existing APIs.

## Connections and calibration

| Input | Pin | ADC channel |
| --- | --- | --- |
| J1X | PA0 | IN0 |
| J1Y | PA1 | IN1 |
| J2X | PA2 | IN2 |
| J2Y | PA3 | IN3 |

ADC1 uses software-triggered single conversions with 71.5-cycle sampling.
`Joystick_Start()` calibrates the ADC once at startup. `joystick_config.h` holds
the imported centres/endpoints, deadzone and Y directions. Physical joystick
calibration still needs measuring. Failed ADC acquisition produces zero axes.

Button bits 0 through 7 correspond to Button1 through Button8. The existing
board convention treats a LOW input as pressed. Joystick push switches and the
LimitSwitch-labelled inputs are not included. USART1 uses PB6 TX / PB7 RX,
115200 baud, 8N1; confirm the HC-05 settings and physical connections.

## Packet: 10 bytes

| Offset | Value |
| --- | --- |
| 0 | Eight buttons: bit 0 = Button1, ..., bit 7 = Button8; 1 = pressed |
| 1-2 | J1X |
| 3-4 | J1Y |
| 5-6 | J2X |
| 7-8 | J2Y |
| 9 | CRC-8 over bytes 0 through 8 |

Each axis is a signed 16-bit integer, low byte first, rounded to [-1000, +1000].
For example, -1000 represents -1.0 and 500 represents +0.5. There are no start
markers or additional fields. The mainboard receiver must use this ten-byte
layout, replacing the earlier fifteen-byte format.

CRC-8 settings: polynomial 0x07, initial value 0x00, no reflection, final XOR
0x00. The ASCII bytes `123456789` produce 0xF4.

## TFT

The screen shows Button1 to Button8 from left to right as eight 0/1 characters,
four signed axis values in the same scale as the packet, and that packet's CRC.
No floating-point TFT formatting is needed.

## Build

From the Controller folder:

```powershell
cmake --preset Debug
cmake --build --preset Debug
```

The firmware is `build/Debug/Controller.elf`. Physical TFT output, calibration
and HC-05 transmission still need checking on the actual controller. MainBoard
firmware has not been changed.

## Original resources

- [Controller PCB](https://github.com/UST-Robotics-Team/Software-Tutorial-2026-Notes/blob/main/additional-notes/RDC-controller.md)
- [Tutorial 2: UART and Bluetooth](https://github.com/UST-Robotics-Team/Software-Tutorial-2026-Notes/tree/main/tutorial-2-uart-bluetooth)
- [ADC](https://github.com/UST-Robotics-Team/Software-Tutorial-2026-Notes/blob/main/advanced-tutorial-1-Advanced-Embedded-Systems/notes/01-adc.md)
- [Joystick example](https://github.com/UST-Robotics-Team/Software-Tutorial-2026-Notes/blob/main/advanced-tutorial-1-Advanced-Embedded-Systems/notes/02-joystick.md)
