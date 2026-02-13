// 07_radio_rx_drive_motor_stepper_dfplayer.ino
// Stage 07: nRF24 receiver drives DC motor (L293D) + steering stepper (ULN2003)
//           AND plays an MP3 sound effect on "fire" (DFPlayer Mini).
//
// DFPlayer wiring reminder (critical fix):
//   DF TX  -> Nano D7
//   DF RX  <- Nano D8 THROUGH 1k (or 2.2k) resistor
//   DF BUSY-> Nano D6 (LOW=playing on most boards)
//   Speaker-> SPK1 / SPK2
//   VCC/GND-> 5V/GND (common ground)

#include <SPI.h>
#include <RF24.h>
#include <SoftwareSerial.h>
#include <DFRobotDFPlayerMini.h>

// ---------------- Radio ----------------
RF24 radio(9, 10);               // CE, CSN
const byte address[6] = "JOY1";

struct Payload {
  int16_t x;       // -512..+511
  int16_t y;       // -512..+511
  uint16_t seq;
  uint8_t fire;    // 0/1 (button)
};

// ---------------- Motor (L293D) ----------------
const uint8_t PIN_EN  = 5;  // PWM (L293D pin 1 EN1,2)
const uint8_t PIN_IN1 = 4;  // L293D pin 2 IN1
const uint8_t PIN_IN2 = 3;  // L293D pin 7 IN2

// NEW: minimum PWM once motor is engaged (reduces audible "chirp" at low duty)
const uint8_t PWM_MIN = 45; // try 35–60 if needed

// ---------------- Stepper (ULN2003 + 28BYJ-48) ----------------
const uint8_t ST_IN1 = A0;
const uint8_t ST_IN2 = A1;
const uint8_t ST_IN3 = A2;
const uint8_t ST_IN4 = A3;

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
const int DEAD_X = 60;
const int DEAD_Y = 40;
const unsigned long FAILSAFE_MS = 400;

// Progressive steering timing (ms per half-step)
const unsigned long STEER_INTERVAL_SLOW_MS = 14;
const unsigned long STEER_INTERVAL_FAST_MS = 1;

// Soft steering limits (half-steps)
const int16_t STEER_MIN = -800;
const int16_t STEER_MAX = +800;

unsigned long lastRxMs = 0;

// Stepper state
int8_t  steerDir  = 0;
int16_t steerPos  = 0;
uint8_t stepIndex = 0;
unsigned long lastStepMs = 0;

// ---------------- DFPlayer ----------------
const uint8_t DF_BUSY_PIN = 6;  // DFPlayer BUSY -> D6
const uint8_t DF_RX_PIN   = 7;  // Nano receives  (DF TX -> D7)
const uint8_t DF_TX_PIN   = 8;  // Nano transmits (D8 -> 1k -> DF RX)

SoftwareSerial dfSerial(DF_RX_PIN, DF_TX_PIN);
DFRobotDFPlayerMini dfPlayer;
bool dfOk = false;

const uint8_t  SFX_TRACK_MP3_FOLDER = 1;      // /MP3/0001.mp3
const unsigned long FIRE_LOCKOUT_MS  = 600;   // debounce / anti-spam
unsigned long lastFireMs = 0;
uint8_t lastFire = 0;

bool dfIsPlaying() {
  // Most DFPlayers: BUSY LOW while playing
  return digitalRead(DF_BUSY_PIN) == LOW;
}

void playFireSfx() {
  if (!dfOk) return;

  // Restart cleanly
  dfPlayer.stop();
  delay(120);
  dfPlayer.playMp3Folder(SFX_TRACK_MP3_FOLDER);
}

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

  // NEW: avoid tiny PWM pulses right after deadband (reduces audible "chirp")
  if (pwm > 0 && pwm < PWM_MIN) pwm = PWM_MIN;

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

void stepperHold() {
  steerDir = 0;
}

int8_t joyToSteerDir(int16_t x) {
  if (abs(x) < DEAD_X) return 0;
  return (x > 0) ? +1 : -1;
}

// Non-linear progressive steering: squared curve.
unsigned long steeringIntervalFromX(int16_t x) {
  long mag = abs(x);
  mag = constrain(mag, (long)DEAD_X, 512L);

  // Normalize to 0..1000
  long t = (mag - (long)DEAD_X) * 1000L / (512L - (long)DEAD_X);

  // Square for non-linear ramp: stays slow longer, speeds up more near the end
  long t2 = (t * t) / 1000L; // still 0..1000

  long slow = (long)STEER_INTERVAL_SLOW_MS;
  long fast = (long)STEER_INTERVAL_FAST_MS;

  // Interpolate: interval = slow - (slow-fast)*t2
  long interval = slow - ((slow - fast) * t2) / 1000L;

  if (interval < fast) interval = fast;
  if (interval > slow) interval = slow;
  return (unsigned long)interval;
}

void stepperService(unsigned long nowMs, int16_t xValue) {
  if (steerDir == 0) return;

  unsigned long dynamicInterval = steeringIntervalFromX(xValue);
  if (nowMs - lastStepMs < dynamicInterval) return;

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
  // Serial.println("Stage 07 RX motor + stepper + DFPlayer starting");

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

  // DFPlayer pins
  pinMode(DF_BUSY_PIN, INPUT_PULLUP);
  dfSerial.begin(9600);

  // Try init DFPlayer but don't brick the rover if it fails
  if (dfPlayer.begin(dfSerial)) {
    dfOk = true;
    dfPlayer.volume(16);                  // 0..30
    dfPlayer.EQ(DFPLAYER_EQ_NORMAL);
  } else {
    dfOk = false;
  }

  // Radio
  radio.begin();
  radio.setPALevel(RF24_PA_LOW);
  radio.setDataRate(RF24_250KBPS);
  radio.setChannel(108);
  radio.openReadingPipe(1, address);
  radio.startListening();

  lastRxMs = millis();
}

void loop() {
  static Payload lastP = {0, 0, 0, 0};     // keep last packet so steering can keep using last X
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
    lastP = p;
    lastRxMs = nowMs;

    // Motor from Y (signed)
    motorDriveSigned(p.y);

    // Steering from X (while held)
    steerDir = joyToSteerDir(p.x);

    // Fire sound: rising edge + lockout
    if (p.fire && !lastFire && (nowMs - lastFireMs > FIRE_LOCKOUT_MS)) {
      playFireSfx();
      lastFireMs = nowMs;
    }
    lastFire = p.fire;
  }

  // Failsafe: if link drops, stop motor & hold steering
  if (nowMs - lastRxMs > FAILSAFE_MS) {
    failsafeStop();
  }

  // Non-blocking stepper update (always runs)
  stepperService(nowMs, lastP.x);
}