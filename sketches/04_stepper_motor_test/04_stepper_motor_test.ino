/*
  28BYJ-48 Stepper Motor Test
  Using ULN2003 driver board
  Pins: A0, A1, A2, A3

  Power:
  - ULN2003 VCC -> 5V (from MB102 5V side)
  - ULN2003 GND -> MB102 GND
  - MB102 GND -> Nano GND (IMPORTANT)
*/

const int IN1 = A0;
const int IN2 = A1;
const int IN3 = A2;
const int IN4 = A3;

// 28BYJ-48: 2048 steps per full revolution (half-step mode)
const int STEPS_PER_REV = 2048;

int stepDelay = 3;  // lower = faster (try 2–5)

void setup() {
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
}

void loop() {
  stepForward(STEPS_PER_REV);
  delay(1000);
  stepBackward(STEPS_PER_REV);
  delay(1000);
}

void stepForward(int steps) {
  for (int i = 0; i < steps; i++) {
    stepMotor(i % 8);
    delay(stepDelay);
  }
}

void stepBackward(int steps) {
  for (int i = 0; i < steps; i++) {
    stepMotor(7 - (i % 8));
    delay(stepDelay);
  }
}

void stepMotor(int step) {
  switch(step) {
    case 0: setCoils(1,0,0,0); break;
    case 1: setCoils(1,1,0,0); break;
    case 2: setCoils(0,1,0,0); break;
    case 3: setCoils(0,1,1,0); break;
    case 4: setCoils(0,0,1,0); break;
    case 5: setCoils(0,0,1,1); break;
    case 6: setCoils(0,0,0,1); break;
    case 7: setCoils(1,0,0,1); break;
  }
}

void setCoils(int a, int b, int c, int d) {
  digitalWrite(IN1, a);
  digitalWrite(IN2, b);
  digitalWrite(IN3, c);
  digitalWrite(IN4, d);
}
