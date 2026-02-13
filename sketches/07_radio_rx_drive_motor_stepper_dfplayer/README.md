# Stage 07 – RX: Motor + Stepper + DFPlayer Integration

This stage combines:

- nRF24L01 radio receiver
- DC motor via L293D
- 28BYJ-48 stepper via ULN2003
- DFPlayer Mini MP3 sound effect (fire button trigger)

---

## Arduino Nano Pin Usage Summary

| Nano Pin | Connected To |
|-----------|--------------|
| D3 | L293D pin 7 (IN2) |
| D4 | L293D pin 2 (IN1) |
| D5 | L293D pin 1 (EN1,2 PWM) |
| D6 | DFPlayer BUSY |
| D7 | DFPlayer TX |
| D8 | DFPlayer RX (through 1k–2.2k resistor) |
| D9 | nRF24 CE |
| D10 | nRF24 CSN |
| D11 | nRF24 MOSI |
| D12 | nRF24 MISO |
| D13 | nRF24 SCK |
| A0–A3 | ULN2003 stepper inputs |

---

## L293D (DC Motor Driver)

| L293D Pin | Connection |
|------------|------------|
| 1 | Nano D5 (PWM enable) |
| 2 | Nano D4 (IN1) |
| 3 | Motor lead A |
| 4,5 | GND |
| 6 | Motor lead B |
| 7 | Nano D3 (IN2) |
| 8 | +5V motor supply |
| 9 | GND (disable unused channel) |
| 12,13 | GND |
| 16 | +5V logic |

### Recommended Decoupling
- 100nF ceramic from pin 16 to GND
- 100nF ceramic from pin 8 to GND
- 100nF ceramic across motor terminals

---

## ULN2003 + 28BYJ-48 Stepper

| ULN2003 IN | Nano Pin |
|------------|----------|
| IN1 | A0 |
| IN2 | A1 |
| IN3 | A2 |
| IN4 | A3 |

ULN2003:
- VCC → 5V
- GND → GND

---

## DFPlayer Mini

### Power
- VCC → 5V
- GND → GND
- 100nF ceramic across VCC/GND
- 470µF–1000µF electrolytic across VCC/GND

### Serial Control
- DF TX → Nano D7
- Nano D8 → 1k–2.2k resistor → DF RX
- DF BUSY → Nano D6

**Important:** The resistor on DF RX is required to prevent buzzing/noise and protect the module.

### Speaker
- Connect speaker directly to SPK1 and SPK2
- Do NOT connect speaker to GND

### SD Card
- FAT32 formatted
- Folder: /MP3
- File: 0001.mp3

---

## Power Architecture

- 5V rail (MB102) powers:
  - L293D
  - ULN2003
  - DFPlayer
  - Nano (via 5V pin)
- 3.3V rail powers:
  - nRF24L01 (with decoupling capacitor)

All grounds must be common.
