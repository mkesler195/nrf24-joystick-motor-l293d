# 06 DFPlayer RX Smoke Test

## Purpose

This sketch isolates and verifies DFPlayer Mini functionality on the RX unit before integrating it with:

- nRF24 radio
- L293D DC motor control
- 28BYJ-48 stepper control
- Failsafe logic

It confirms:
- Proper power wiring
- SD card formatting
- Speaker wiring
- Stable playback under 5V supply

---

## Hardware Wiring (Nano)

### Power
- DFPlayer VCC → 5V (MB102 5V rail)
- DFPlayer GND → Common GND (shared with Nano and motor drivers)

### Decoupling (Important)
Place near DFPlayer:
- 100–470uF electrolytic across VCC/GND
- 0.1uF ceramic (100nF, marked "104") across VCC/GND

### Serial (SoftwareSerial)
- DFPlayer TX → Nano D7
- DFPlayer RX ← Nano D8 (through 1k series resistor)

### Speaker
- Speaker lead → SPK1
- Speaker lead → SPK2
- Do NOT connect speaker to GND (bridge output)

---

## SD Card Layout

- Format: FAT32
- Folder: MP3
- File: 0001.mp3

Full path on the card:

    /MP3/0001.mp3

---

## Expected Behavior

- On boot, the sketch initializes DFPlayer.
- Volume is set to a moderate level.
- /MP3/0001.mp3 plays once.
- The track repeats every 5 seconds.

---

## Troubleshooting

If DFPlayer fails to initialize:
- Confirm SD card is FAT32
- Confirm folder name is exactly MP3
- Confirm file name is exactly 0001.mp3
- Check TX/RX wiring is not reversed
- Verify 1k resistor on Nano → DFPlayer RX line
- Increase electrolytic capacitor if unstable

---

## Status

Stage 06: DFPlayer isolated validation before full RX integration.≈
