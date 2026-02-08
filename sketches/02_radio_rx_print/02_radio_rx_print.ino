#include <SPI.h>
#include <RF24.h>

RF24 radio(9, 10);              // CE, CSN
const byte address[6] = "JOY1";

struct Payload {
  int16_t x;
  int16_t y;
  uint16_t seq;
};

void setup() {
  Serial.begin(115200);
  Serial.println("RX starting");

  radio.begin();
  radio.setPALevel(RF24_PA_LOW);
  radio.setDataRate(RF24_250KBPS);
  radio.setChannel(108);

  radio.openReadingPipe(1, address);
  radio.startListening();

  Serial.println("RX ready");
}

void loop() {
  Payload p;

  if (radio.available()) {
    while (radio.available()) {
      radio.read(&p, sizeof(p));
    }

    Serial.print("seq=");
    Serial.print(p.seq);
    Serial.print(" x=");
    Serial.print(p.x);
    Serial.print(" y=");
    Serial.println(p.y);
  }
}
