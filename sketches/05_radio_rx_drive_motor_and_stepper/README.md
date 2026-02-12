# Stage 5 — RX: DC Motor + Stepper Steering

## Overview

This stage integrates:

- nRF24L01 wireless receiver
- DC motor driven by L293D
- 28BYJ-48 stepper motor driven by ULN2003
- Soft steering limits
- Radio failsafe protection

This replaces the latched-direction logic from Stage 3 with real-time steering control.

---

## Control Behavior

### Joystick Y (Forward / Back)
- Controls DC motor direction and speed
- Deadband near center stops motor (coast)

### Joystick X (Left / Right)
- Controls steering stepper
- Stepper continues stepping while stick is held
- Center position = hold position

### Failsafe
If no radio packet is received for FAILSAFE_MS:
- Motor stops (coast)
- Stepper holds current position

---

## Pin Mapping

### nRF24L01

| Signal | Nano Pin |
|--------|----------|
| CE     | D9       |
| CSN    | D10      |
| MOSI   | D11      |
| MISO   | D12      |
| SCK    | D13      |

---

### L293D (DC Motor)

| L293D Pin | Nano Pin |
|------------|----------|
| EN1        | D5 (PWM) |
| IN1        | D4       |
| IN2        | D3       |

---

### ULN2003 (Stepper Steering)

| ULN2003 Pin | Nano Pin |
|-------------|----------|
| IN1         | A0       |
| IN2         | A1       |
| IN3         | A2       |
| IN4         | A3       |

---

## Steering Limits

Soft limits prevent mechanical over-rotation.

Current configuration:

    STEER_MIN = -800
    STEER_MAX = +800

These values represent half-step counts relative to startup center.

Adjust after confirming physical travel range.

---

## Power Notes

- ULN2003 powered from MB102 5V output
- Jumper set to 5V
- Grounds shared between:
  - Nano
  - MB102
  - L293D motor supply
  - nRF24 module

---

## Design Notes

- Stepper control is non-blocking
- Radio responsiveness preserved
- Motor stop behavior is COAST (not brake)
- Steering holds position by keeping coils energized
- Soft limits protect mechanical linkage

---

## Future Enhancements

- Add joystick button to re-center steering
- Add slow ramping acceleration
- Add steering return-to-center on failsafe
- Add current monitoring

