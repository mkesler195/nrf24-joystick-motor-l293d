#include <SPI.h>
#include <RF24.h>

RF24 radio(9, 10);              // CE, CSN
const byte address[6] = "JOY1";

// MUST match RX exactly
struct Payload {
  int16_t x;       // -512..+511
  int16_t y;       // -512..+511
  uint16_t seq;
  uint8_t fire;
};

uint16_t seq = 0;

const uint8_t FIRE_PIN = 2;     // Button to GND

int16_t readAxis(uint8_t pin, int deadband = 40) {
  int v = analogRead(pin);      // 0..1023
  int c = v - 512;              // center around 0
  if (abs(c) < deadband) c = 0;
  return (int16_t)c;
}

void setup() {
  Serial.begin(115200);

  pinMode(FIRE_PIN, INPUT_PULLUP);

  radio.begin();
  radio.setPALevel(RF24_PA_LOW);
  radio.setDataRate(RF24_250KBPS);
  radio.setChannel(108);

  radio.openWritingPipe(address);
  radio.stopListening();

  Serial.println("TX Stage 07 ready");
}

void loop() {
  Payload p;

  p.x = readAxis(A0);         // joystick X
  p.y = -readAxis(A1);        // INVERT Y so forward/back feel correct
  p.seq = seq++;
  p.fire = (digitalRead(FIRE_PIN) == LOW) ? 1 : 0;

  radio.write(&p, sizeof(p));

  // --- TEMP DEBUG (remove later) ---
  Serial.print("x=");
  Serial.print(p.x);
  Serial.print(" y=");
  Serial.print(p.y);
  Serial.print(" fire=");
  Serial.println(p.fire);
  // ---------------------------------

  delay(30); // ~33 Hz
}
