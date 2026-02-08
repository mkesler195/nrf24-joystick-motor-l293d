// 01_motor_only_l293d.ino
// Motor-only bring-up test for Nano + L293D + DC motor
// EN = PWM speed, IN1/IN2 = direction

const uint8_t PIN_EN  = 5;  // must be PWM-capable on Nano
const uint8_t PIN_IN1 = 4;
const uint8_t PIN_IN2 = 3;

void motorCoast() {
  digitalWrite(PIN_IN1, LOW);
  digitalWrite(PIN_IN2, LOW);
  analogWrite(PIN_EN, 0);
}

void motorForward(uint8_t pwm) {
  digitalWrite(PIN_IN1, HIGH);
  digitalWrite(PIN_IN2, LOW);
  analogWrite(PIN_EN, pwm);
}

void motorReverse(uint8_t pwm) {
  digitalWrite(PIN_IN1, LOW);
  digitalWrite(PIN_IN2, HIGH);
  analogWrite(PIN_EN, pwm);
}

void setup() {
  pinMode(PIN_EN, OUTPUT);
  pinMode(PIN_IN1, OUTPUT);
  pinMode(PIN_IN2, OUTPUT);

  motorCoast();
  delay(500);
}

void loop() {
  // Forward ramp up
  for (int pwm = 0; pwm <= 255; pwm += 5) {
    motorForward((uint8_t)pwm);
    delay(30);
  }

  // Forward ramp down
  for (int pwm = 255; pwm >= 0; pwm -= 5) {
    motorForward((uint8_t)pwm);
    delay(30);
  }

  motorCoast();
  delay(600);

  // Reverse ramp up
  for (int pwm = 0; pwm <= 255; pwm += 5) {
    motorReverse((uint8_t)pwm);
    delay(30);
  }

  // Reverse ramp down
  for (int pwm = 255; pwm >= 0; pwm -= 5) {
    motorReverse((uint8_t)pwm);
    delay(30);
  }

  motorCoast();
  delay(1200);
}
