// 03_radio_rx_drive_motor.ino
// Stage 3: nRF24 receiver drives DC motor via L293D from joystick input
// Design intent:
//   - Joystick Y (forward/back) controls SPEED (faster/slower)
//   - Joystick X (left/right) selects DIRECTION (latched)
//   - Failsafe: if radio packets stop, motor stops

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
const uint8_t PIN_EN  = 5;  // PWM (L293D pin 1 EN1,2)
const uint8_t PIN_IN1 = 4;  // L293D pin 2 IN1
const uint8_t PIN_IN2 = 3;  // L293D pin 7 IN2

// ---------------- Control tuning ----------------
const int DEAD_X = 60;               // direction selection deadband (left/right)
const int DEAD_Y = 40;               // speed deadband (forward/back)
const unsigned long FAILSAFE_MS = 400;

unsigned long lastRxMs = 0;

// Latched direction: updated only when X is pushed past DEAD_X
int8_t lastDir = +1;                 // default direction on startup (+1 or -1)

void motorStopCoast() {
  digitalWrite(PIN_IN1, LOW);
  digitalWrite(PIN_IN2, LOW);
  analogWrite(PIN_EN, 0);
}

void motorDrive(int8_t dir, uint8_t pwm) {
  if (pwm == 0) {                     // speed=0 always stops motor
    motorStopCoast();
    return;
  }

  // If direction is ever 0 (shouldn't be after latching), coast
  if (dir == 0) {
    motorStopCoast();
    return;
  }

  if (dir > 0) {                      // "forward"
    digitalWrite(PIN_IN1, HIGH);
    digitalWrite(PIN_IN2, LOW);
  } else {                            // "reverse"
    digitalWrite(PIN_IN1, LOW);
    digitalWrite(PIN_IN2, HIGH);
  }

  analogWrite(PIN_EN, pwm);
}

uint8_t mapJoyToPwm(int16_t y) {
  int v = abs(y);                     // speed is magnitude of Y
  if (v < DEAD_Y) return 0;
  v = constrain(v, DEAD_Y, 512);
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

    // Speed from Y
    uint8_t pwm = mapJoyToPwm(p.y);

    // Direction from X (latched)
    int8_t newDir = joyToDir(p.x);
    if (newDir != 0) lastDir = newDir;

    // Drive motor using latched direction and current speed
    motorDrive(lastDir, pwm);

    // Debug
    Serial.print("seq="); Serial.print(p.seq);
    Serial.print(" x=");  Serial.print(p.x);
    Serial.print(" y=");  Serial.print(p.y);
    Serial.print(" dir="); Serial.print((int)lastDir);
    Serial.print(" pwm="); Serial.println(pwm);
  }

  // Failsafe: if link drops, stop motor
  if (millis() - lastRxMs > FAILSAFE_MS) {
    motorStopCoast();
  }
}
