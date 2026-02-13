# Stage 07 – TX: Joystick + Fire Button

This stage adds a “Fire” button to the joystick transmitter and sends
a `fire` flag to the RX unit over nRF24.

---

## Arduino Nano Pin Usage Summary

| Nano Pin | Connected To |
|-----------|--------------|
| A0 | Joystick VRx |
| A1 | Joystick VRy |
| D2 | Fire Button (to GND) |
| D9 | nRF24 CE |
| D10 | nRF24 CSN |
| D11 | nRF24 MOSI |
| D12 | nRF24 MISO |
| D13 | nRF24 SCK |
| 3.3V | nRF24 VCC |
| 5V | Joystick VCC |
| GND | Common Ground |

---

## nRF24L01 Wiring

| nRF24 Pin | Nano Pin |
|------------|----------|
| CE | D9 |
| CSN | D10 |
| MOSI | D11 |
| MISO | D12 |
| SCK | D13 |
| VCC | 3.3V |
| GND | GND |

### Important
- Add a 10µF–100µF capacitor across nRF24 VCC and GND.
- Do NOT power nRF24 from 5V.

---

## Joystick Wiring

| Joystick Pin | Nano Pin |
|---------------|----------|
| VRx | A0 |
| VRy | A1 |
| VCC | 5V |
| GND | GND |

Joystick readings are centered around 512 and converted to -512…+511.

Y axis is inverted in software so:

- Push forward → positive motor direction
- Pull back → reverse motor direction

---

## Fire Button Wiring

- One side of button → Nano D2
- Other side of button → GND
- Uses `INPUT_PULLUP`

Behavior:
- Not pressed = HIGH
- Pressed = LOW
- TX sends `fire = 1` while pressed

---

## Payload Structure (Must Match RX Exactly)

```cpp
struct Payload {
  int16_t x;
  int16_t y;
  uint16_t seq;
  uint8_t fire;
};
