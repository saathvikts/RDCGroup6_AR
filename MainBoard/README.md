# Mainboard Resources

## Motor 1 proportional speed test

MainBoard now uses BTN1 for +300 rotor RPM, BTN2 for -300 rotor RPM, and neither
for 0 RPM. `RM_MOTOR_1` means ESC ID 1. The motor library reports rotor RPM before
the gearbox, so the target uses those same units.

The P-only calculation is `current = RM_SPEED_KP * (target_rpm - measured_rpm)`.
Its initial, untuned gain is 0.001 A/RPM and its output is limited to +/-1.5 A.
The library's existing 5 A soft limit is unchanged. There is no integral or
derivative term. A zero-speed target can apply braking current while moving.

Current is requested only after valid feedback from this motor has arrived
within 100 ms; otherwise the requested current is zero. The TFT shows target,
measured speed, error, commanded/measured current, encoder and feedback status.
Control uses the existing 1 ms main-loop time check; TFT drawing can introduce
timing jitter. The gain and feedback direction still need checking on a secured
motor before use on the chassis. The program has not been flashed by this task.

## Original resources

> [!CAUTION]
> If ELCP motors are connected, disconnect motor power or keep the wheels
> off the ground while flashing — GPIO lines float briefly during reset.

- [Tutorial 2 -- Bluetooth communication with Controller](https://github.com/UST-Robotics-Team/Software-Tutorial-2026-Notes/tree/main/tutorial-2-uart-bluetooth)
- [Tutorial 3 -- Servo and PWM](https://github.com/UST-Robotics-Team/Software-Tutorial-2026-Notes/tree/main/tutorial-3-advanced-io)
- [ELCP Motors -- library docs](Core/Inc/elcp_motor/README.md)
- [Advanced Tutorial 1 -- IR Sensors](https://github.com/UST-Robotics-Team/Software-Tutorial-2026-Notes/blob/main/advanced-tutorial-1-Advanced-Embedded-Systems/notes/03-ir.md)
- [Advanced Tutorial 1 -- Ultrasonic Sensor](https://github.com/UST-Robotics-Team/Software-Tutorial-2026-Notes/blob/main/advanced-tutorial-1-Advanced-Embedded-Systems/notes/04-ultrasonic.md)
- [Advanced Tutorial 1 -- I2C](https://github.com/UST-Robotics-Team/Software-Tutorial-2026-Notes/blob/main/advanced-tutorial-1-Advanced-Embedded-Systems/notes/05-i2c.md)
- [Advanced Tutorial 1 -- TOF (optional)](https://github.com/UST-Robotics-Team/Software-Tutorial-2026-Notes/blob/main/advanced-tutorial-1-Advanced-Embedded-Systems/notes/06-optional-tof.md)
- [Advanced Tutorial 2 -- PID Control Theory and Motor Control](https://github.com/UST-Robotics-Team/Software-Tutorial-2026-Notes/tree/main/advanced-tutorial-2-CAN)
- Google is your friend! Do your own research and implement the code yourself.
