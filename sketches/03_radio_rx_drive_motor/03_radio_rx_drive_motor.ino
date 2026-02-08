#include <SPI.h>
#include <RF24.h>

// ---------------- Radio ----------------
RF24 radio(9, 10);               // CE, CSN
const byte address[6] = "JOY1";

struct Payload {
  int16_t x;       // -512..+511
  int16_t y;       // -512..+511
  uint16_t seq;
};

// ---------------- Motor (L293D) ----------------
// Match these to your working motor-only wiring:
const uint8_t PIN_EN  = 5;  // PWM
const uint8_t PIN_IN1 = 4;
const uint8_t PIN_IN2 = 3;

// ---------------- Control tuning ----------------
const int DEAD_X = 80;           // direction deadband (left/right)
const int DEAD_Y = 40;           // speed deadband (forward/back)
const unsigned long FAILSAFE_MS = 400;

unsigned long lastRxMs = 0;

void motorStopCoast() {
  digitalWrite(PIN_IN1, LOW);
  digitalWrite(PIN_IN2, LOW);
  analogWrite(PIN_EN, 0);
}

void motorDrive(int8_t dir, uint8_t pwm) {
  if (dir == 0 || pwm == 0) {
    motorStopCoast();
    return;
  }

  if (dir > 0) {
    digitalWrite(PIN_IN1, HIGH);
    digitalWrite(PIN_IN2, LOW);
  } else {
    digitalWrite(PIN_IN1, LOW);
    digitalWrite(PIN_IN2, HIGH);
  }

  analogWrite(PIN_EN, pwm);
}

uint8_t mapJoyToPwm(int16_t y) {
  int v = abs(y);
  if (v < DEAD_Y) return 0;
  v = constrain(v, DEAD_Y, 512);
  // Map from [DEAD_Y..512] to [0..255]
  return (uint8_t)map(v, DEAD_Y, 512, 0, 255);
}

int8_t joyToDir(int16_t x) {
  if (abs(x) < DEAD_X) return 0;
  return (x > 0) ? +1 : -1;
}

void setup() {
  Serial.begin(115200);
  Serial.println("Stage 3 RX motor starting");

  // Motor pins
  pinMode(PIN_EN, OUTPUT);
  pinMode(PIN_IN1, OUTPUT);
  pinMode(PIN_IN2, OUTPUT);
  motorStopCoast();

  // Radio
  radio.begin();
  radio.setPALevel(RF24_PA_LOW);
  radio.setDataRate(RF24_250KBPS);
  radio.setChannel(108);
  radio.openReadingPipe(1, address);
  radio.startListening();

  Serial.println("Stage 3 RX ready");
}

void loop() {
  Payload p;

  if (radio.available()) {
    while (radio.available()) {
      radio.read(&p, sizeof(p));
    }
    lastRxMs = millis();

    uint8_t pwm = mapJoyToPwm(p.y);
    int8_t dir  = joyToDir(p.x);

    motorDrive(dir, pwm);

    // Debug (optional)
    Serial.print("seq="); Serial.print(p.seq);
    Serial.print(" x="); Serial.print(p.x);
    Serial.print(" y="); Serial.print(p.y);
    Serial.print(" dir="); Serial.print((int)dir);
    Serial.print(" pwm="); Serial.println(pwm);
  }

  // Failsafe: if link drops, stop motor
  if (millis() - lastRxMs > FAILSAFE_MS) {
    motorStopCoast();
  }
}
