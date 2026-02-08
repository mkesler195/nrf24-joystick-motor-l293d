#include <SPI.h>
#include <RF24.h>

RF24 radio(9, 10);              // CE, CSN
const byte address[6] = "JOY1";

struct Payload {
  int16_t x;      // centered joystick X
  int16_t y;      // centered joystick Y
  uint16_t seq;   // packet counter
};

uint16_t seq = 0;

int16_t readAxis(uint8_t pin, int deadband = 40) {
  int v = analogRead(pin);      // 0..1023
  int c = v - 512;              // -512..+511
  if (abs(c) < deadband) c = 0;
  return (int16_t)c;
}

void setup() {
  Serial.begin(115200);
  Serial.println("TX starting");

  radio.begin();
  radio.setPALevel(RF24_PA_LOW);
  radio.setDataRate(RF24_250KBPS);
  radio.setChannel(108);

  radio.openWritingPipe(address);
  radio.stopListening();

  Serial.println("TX ready");
}

void loop() {
  Payload p;
  p.x   = readAxis(A0);   // joystick X
  p.y   = readAxis(A1);   // joystick Y
  p.seq = seq++;

  bool ok = radio.write(&p, sizeof(p));

  Serial.print("seq=");
  Serial.print(p.seq);
  Serial.print(" x=");
  Serial.print(p.x);
  Serial.print(" y=");
  Serial.print(p.y);
  Serial.print(" ok=");
  Serial.println(ok);

  delay(30); // ~33 Hz update rate
}
