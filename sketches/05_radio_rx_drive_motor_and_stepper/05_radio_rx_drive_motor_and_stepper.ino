// 05_radio_rx_drive_motor_and_stepper.ino
// Stage 4: nRF24 receiver drives DC motor via L293D AND steering stepper via ULN2003
// Design intent:
//   - Joystick Y (forward/back) controls DC motor DIRECTION + SPEED
//   - Joystick X (left/right) controls STEPPER steering (keeps stepping while held)
//   - Joystick centered: motor STOP (coast), stepper HOLD
//   - Failsafe: if radio packets stop, motor stops, stepper holds
//   - Soft steering limits prevent over-rotation

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
const uint8_t PIN_EN  = 5;  // PWM (L293D pin 1 EN1,2)
const uint8_t PIN_IN1 = 4;  // L293D pin 2 IN1
const uint8_t PIN_IN2 = 3;  // L293D pin 7 IN2

// ---------------- Stepper (ULN2003 + 28BYJ-48) ----------------
// You agreed: A0, A1, A2, A3
const uint8_t ST_IN1 = A0;
const uint8_t ST_IN2 = A1;
const uint8_t ST_IN3 = A2;
const uint8_t ST_IN4 = A3;

// Half-step sequence (smooth steering)
const uint8_t HALFSTEP_SEQ[8][4] = {
  {1,0,0,0},
  {1,1,0,0},
  {0,1,0,0},
  {0,1,1,0},
  {0,0,1,0},
  {0,0,1,1},
  {0,0,0,1},
  {1,0,0,1}
};

// ---------------- Control tuning ----------------
const int DEAD_X = 60;                 // steering deadband (left/right)
const int DEAD_Y = 40;                 // motor deadband (forward/back)
const unsigned long FAILSAFE_MS = 400;

// Stepper steering behavior
const unsigned long STEP_INTERVAL_MS = 3;  // ms between half-steps (3 felt good in your test)
const int16_t STEER_MIN = -800;            // soft limit (half-steps)
const int16_t STEER_MAX = +800;            // soft limit (half-steps)

unsigned long lastRxMs = 0;

// Stepper state
int8_t steerDir = 0;                      // -1 = left, 0 = hold, +1 = right
int16_t steerPos = 0;                     // current position (half-steps), 0 = "center"
uint8_t stepIndex = 0;                    // 0..7 sequence index
unsigned long lastStepMs = 0;

// ---------------- Motor helpers ----------------
void motorStopCoast() {
  digitalWrite(PIN_IN1, LOW);
  digitalWrite(PIN_IN2, LOW);
  analogWrite(PIN_EN, 0);
}

void motorDriveSigned(int16_t y) {
  int v = abs(y);
  if (v < DEAD_Y) {                       // centered => stop
    motorStopCoast();
    return;
  }

  v = constrain(v, DEAD_Y, 512);
  uint8_t pwm = (uint8_t)map(v, DEAD_Y, 512, 0, 255);

  // Direction from sign of Y
  if (y > 0) {                            // forward
    digitalWrite(PIN_IN1, HIGH);
    digitalWrite(PIN_IN2, LOW);
  } else {                                // reverse
    digitalWrite(PIN_IN1, LOW);
    digitalWrite(PIN_IN2, HIGH);
  }

  analogWrite(PIN_EN, pwm);
}

// ---------------- Stepper helpers ----------------
void stepperApplyPhase(uint8_t idx) {
  digitalWrite(ST_IN1, HALFSTEP_SEQ[idx][0]);
  digitalWrite(ST_IN2, HALFSTEP_SEQ[idx][1]);
  digitalWrite(ST_IN3, HALFSTEP_SEQ[idx][2]);
  digitalWrite(ST_IN4, HALFSTEP_SEQ[idx][3]);
}

// Hold means: stop stepping, keep last energized phase (torque holds position)
void stepperHold() {
  steerDir = 0;
}

int8_t joyToSteerDir(int16_t x) {
  if (abs(x) < DEAD_X) return 0;
  return (x > 0) ? +1 : -1;
}

void stepperService(unsigned long nowMs) {
  if (steerDir == 0) return;
  if (nowMs - lastStepMs < STEP_INTERVAL_MS) return;

  // Soft limits
  if (steerDir > 0 && steerPos >= STEER_MAX) return;
  if (steerDir < 0 && steerPos <= STEER_MIN) return;

  if (steerDir > 0) {                     // right
    stepIndex = (stepIndex + 1) & 0x07;
    steerPos++;
  } else {                                // left
    stepIndex = (stepIndex + 7) & 0x07;   // -1 mod 8
    steerPos--;
  }

  stepperApplyPhase(stepIndex);
  lastStepMs = nowMs;
}

void failsafeStop() {
  motorStopCoast();
  stepperHold();
}

// ---------------- Setup / loop ----------------
void setup() {
  Serial.begin(115200);
  Serial.println("Stage 4 RX motor + stepper starting");

  // Motor pins
  pinMode(PIN_EN, OUTPUT);
  pinMode(PIN_IN1, OUTPUT);
  pinMode(PIN_IN2, OUTPUT);
  motorStopCoast();

  // Stepper pins
  pinMode(ST_IN1, OUTPUT);
  pinMode(ST_IN2, OUTPUT);
  pinMode(ST_IN3, OUTPUT);
  pinMode(ST_IN4, OUTPUT);
  stepperApplyPhase(stepIndex);           // energize initial phase (holds)

  // Radio
  radio.begin();
  radio.setPALevel(RF24_PA_LOW);
  radio.setDataRate(RF24_250KBPS);
  radio.setChannel(108);
  radio.openReadingPipe(1, address);
  radio.startListening();

  lastRxMs = millis();
  Serial.println("Stage 4 RX ready");
}

void loop() {
  Payload p;
  unsigned long nowMs = millis();

  bool got = false;

  if (radio.available()) {
    while (radio.available()) {
      radio.read(&p, sizeof(p));          // keep newest packet
    }
    got = true;
  }

  if (got) {
    lastRxMs = nowMs;

    // Motor from Y (signed)
    motorDriveSigned(p.y);

    // Steering from X (while held)
    steerDir = joyToSteerDir(p.x);

    // Debug
    Serial.print("seq="); Serial.print(p.seq);
    Serial.print(" x=");  Serial.print(p.x);
    Serial.print(" y=");  Serial.print(p.y);
    Serial.print(" steerDir="); Serial.print((int)steerDir);
    Serial.print(" steerPos="); Serial.print(steerPos);
    Serial.println();
  }

  // Failsafe: if link drops, stop motor & hold steering
  if (nowMs - lastRxMs > FAILSAFE_MS) {
    failsafeStop();
  }

  // Non-blocking stepper update (always runs)
  stepperService(nowMs);
}
