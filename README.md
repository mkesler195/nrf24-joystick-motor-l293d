# nRF24 Joystick → DC Motor Control (Arduino Nano + L293D)

This project demonstrates **wireless motor control** using two Arduino Nanos and nRF24L01 radios.  
A handheld joystick unit sends control data over nRF24 to a remote Nano, which drives a DC motor through an L293D motor driver.

The project was built intentionally in **clear bring-up stages**:
1) motor driver + motor only  
2) radio communications only (TX/RX print)  
3) integrated radio-controlled motor with failsafe  

This structure makes the system easy to debug, modify, and extend.

---

## What This Project Does

- Uses a **2-axis joystick** as a control input
- Sends joystick data wirelessly using **nRF24L01**
- Controls a **DC motor’s speed and direction** via an **L293D**
- Includes a **failsafe**: if radio packets stop, the motor stops
- Separates hardware validation into staged sketches

Control mapping used:
- **Joystick Y (forward/back)** → motor speed  
- **Joystick X (left/right)** → motor direction (latched)

---

## Hardware Used

- 2 × Arduino Nano  
- 2 × nRF24L01 radio modules  
- 1 × L293D motor driver IC  
- 1 × small DC motor + propeller  
- 1 × 2-axis joystick module (VRx / VRy)  
- MB102 breadboard power supply module (5V rail)  
- Breadboard + jumper wires  

Recommended (and used):
- **10–47µF electrolytic capacitor** across each nRF24 VCC↔GND (close to module)
- **0.1µF ceramic capacitor** across the motor terminals (noise suppression)

---

## Repository Layout

```
nrf24-joystick-motor-l293d/
├─ README.md
├─ sketches/
│  ├─ 01_motor_only_l293d/
│  │  └─ 01_motor_only_l293d.ino
│  ├─ 02_radio_tx_joystick/
│  │  └─ 02_radio_tx_joystick.ino
│  ├─ 02_radio_rx_print/
│  │  └─ 02_radio_rx_print.ino
│  └─ 03_radio_rx_drive_motor/
│     └─ 03_radio_rx_drive_motor.ino
├─ docs/
└─ assets/
```

Each sketch folder is self-contained and represents one bring-up stage.



Each sketch folder is self-contained and represents one bring-up stage.

---

## Power Notes (Important Constraint)

> **nRF24 Power Constraint (confirmed experimentally):**  
> In this setup, the nRF24L01 modules are **only reliable when powered from the Arduino Nano’s 3.3V pin**.  
> Other 3.3V sources (including MB102 3.3V) were not reliable in this environment.

Rules followed in this project:
- nRF24 **VCC → Nano 3.3V**
- Add **10–47µF capacitor** across nRF24 VCC↔GND close to the module
- Motor is **not powered from the Nano 5V pin**
- All grounds are common

---

## Wiring Summary

### nRF24L01 → Arduino Nano (both TX and RX)

| nRF24 Pin | Nano Pin |
|----------:|----------|
| GND | GND |
| VCC | 3.3V (Nano pin) |
| CE | D9 |
| CSN | D10 |
| SCK | D13 |
| MOSI | D11 |
| MISO | D12 |

**Known-good radio settings:**
- Address: `"JOY1"`
- Channel: `108`
- Data rate: `RF24_250KBPS`
- PA level: `RF24_PA_LOW`

---

### Joystick → TX Nano

Typical joystick wiring:
- VCC → 5V
- GND → GND
- VRx / VRy → A0 / A1

> Note: In this build, joystick axes were discovered to be swapped relative to expectations.  
> This was corrected **in the TX sketch** by swapping which analog pin maps to X vs Y.

---

### L293D + Motor → RX Nano

**Power**
- L293D pin 16 (Vcc1, logic) → 5V
- L293D pin 8  (Vcc2, motor) → motor supply (5V used in test)
- L293D pins 4,5,12,13 → GND

**Control (from RX Nano)**
- L293D pin 1  (EN1,2) → D5 (PWM)
- L293D pin 2  (IN1) → D4
- L293D pin 7  (IN2) → D3

**Motor outputs**
- L293D pin 3 → motor terminal A
- L293D pin 6 → motor terminal B

Add:
- 0.1µF ceramic capacitor across motor terminals

---

## Sketches / Bring-Up Stages

### Stage 1 — Motor Only
**`01_motor_only_l293d.ino`**

Purpose:
- Validate L293D wiring and power
- Verify PWM speed control and direction switching
- Ensure Nano does not reset when motor starts/reverses

Expected:
- Motor ramps forward → stops → ramps reverse → repeats

---

### Stage 2 — Radio Only (Print Packets)
**TX:** `02_radio_tx_joystick.ino`  
**RX:** `02_radio_rx_print.ino`

Purpose:
- Validate stable nRF24 communications
- Confirm joystick readings are correct

Expected:
- TX serial shows `TX ready` and changing x/y values
- RX serial shows steady packet updates with increasing `seq`

---

### Stage 3 — Radio Drives Motor (Integrated)
**RX:** `03_radio_rx_drive_motor.ino`

Purpose:
- Use joystick to control motor wirelessly
- Implement direction latch and speed control
- Add failsafe on radio loss

Behavior:
- Left/right selects direction (latched)
- Forward/back controls speed
- Center stops motor
- TX powered off → motor stops within ~400 ms

---

## Known Behaviors / Design Choices

- Speed uses **absolute Y value** (forward and back both increase speed)
- Direction is **latched** from X so diagonal motion is not required
- Deadbands are used to avoid jitter near joystick center
- Direction can be changed while stopped (configurable in code)

---

## Troubleshooting Notes

- **Motor only runs on diagonals:** fixed by latching direction instead of requiring X+Y simultaneously
- **Forward/back does nothing:** joystick axes swapped → fixed in TX sketch
- **Radio flaky:** confirm nRF24 on Nano 3.3V + capacitor
- **Nano resets when motor runs:** motor must not draw power through Nano; add motor noise suppression

---

## Possible Next Improvements

- Add joystick button as an “ARM / DISARM” safety
- Add LEDs for direction and link status
- Add exponential speed curve or minimum PWM threshold
- Upgrade motor driver (TB6612FNG / DRV8833) to reduce voltage drop
- Add second motor (L293D has a second H-bridge)

---

## Status

✅ Wireless joystick control of DC motor achieved  
✅ Stable radio link  
✅ Clean staged bring-up  
✅ Failsafe implemented

