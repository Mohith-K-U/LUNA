
/* 
This is the code for LUNA, a semi-humanoid robot
Author name: Mohith K U
Author email: mohithmandanna29@gmail.com
*/

#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>


// Servo (hands & neck)

Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver(0x40);

// RIGHT arm channels
const int RIGHT_SHOULDER = 15;
const int RIGHT_ELBOW    = 14;

// LEFT arm channels
const int LEFT_SHOULDER  = 0;
const int LEFT_ELBOW     = 1;

// Neck channel
#define NECK_SERVO 4
#define SERVOMIN  150
#define SERVOMAX  600

int currentAngle[16];  
int neckAngle = 30;    

int angleToPulse(int angle) {
  return map(angle, 0, 180, SERVOMIN, SERVOMAX);
}


// servo control

void moveServoSmooth(int channel, int targetAngle) {
  int step = (targetAngle > currentAngle[channel]) ? 2 : -2;
  int delayTime = 50; 
  while (currentAngle[channel] != targetAngle) {
    currentAngle[channel] += step;
    if ((step > 0 && currentAngle[channel] > targetAngle) ||
        (step < 0 && currentAngle[channel] < targetAngle)) {
      currentAngle[channel] = targetAngle;
    }
    pwm.setPWM(channel, 0, angleToPulse(currentAngle[channel]));
    delay(delayTime);
  }
}

void moveNeckSmooth(int targetAngle) {
  int start = neckAngle;
  int steps = 40;
  float delta = (targetAngle - start) / (float)steps;
  int delayTime = 15;

  for (int i = 0; i <= steps; i++) {
    float newAngle = start + delta * i;
    pwm.setPWM(NECK_SERVO, 0, angleToPulse((int)newAngle));
    delay(delayTime);
  }
  neckAngle = targetAngle;
}


// Handshake routine

void rHandshake() {
  moveServoSmooth(RIGHT_ELBOW, 75);
  delay(400);
  moveServoSmooth(RIGHT_SHOULDER, 45);
  delay(600);

  for (int i = 0; i < 3; i++) {
    moveServoSmooth(RIGHT_ELBOW, 90);
    delay(250);
    moveServoSmooth(RIGHT_ELBOW, 75);
    delay(250);
  }

  moveServoSmooth(RIGHT_ELBOW, 90);
  delay(300);
  moveServoSmooth(RIGHT_SHOULDER, 0);
}


// Hands oscillation

void handsFront() {
  moveServoSmooth(RIGHT_ELBOW, 75);
  moveServoSmooth(LEFT_ELBOW, 75);
  delay(100);
  moveServoSmooth(RIGHT_ELBOW, 90);
  moveServoSmooth(LEFT_ELBOW, 90);
  delay(100);
}


// Motor driver (BTS7960) setup

const int RPWM_L = 8;
const int LPWM_L = 10;
const int REN_L  = 7;
const int LEN_L  = 6;

const int RPWM_R = 9;
const int LPWM_R = 11;
const int REN_R  = 12;
const int LEN_R  = 13;

int speedValue = 160;
unsigned long motorStartTime = 0;
bool moving = false;
enum MotorAction { NONE, FORWARD, BACKWARD, LEFT, RIGHT };
MotorAction currentAction = NONE;


// Motor functions

void moveForward() {
  analogWrite(RPWM_L, speedValue); analogWrite(LPWM_L, 0);
  analogWrite(RPWM_R, 0); analogWrite(LPWM_R, speedValue);
  moving = true; currentAction = FORWARD;
  motorStartTime = millis();
  Serial.println("Moving FORWARD");
}

void moveBackward() {
  analogWrite(RPWM_L, 0); analogWrite(LPWM_L, speedValue);
  analogWrite(RPWM_R, speedValue); analogWrite(LPWM_R, 0);
  moving = true; currentAction = BACKWARD;
  motorStartTime = millis();
  Serial.println("Moving BACKWARD");
}

void turnLeft() {
  moveNeckSmooth(60);
  analogWrite(RPWM_L, 0); analogWrite(LPWM_L, 0);     // stop left
  analogWrite(RPWM_R, 0); analogWrite(LPWM_R, speedValue); // right forward
  moving = true; currentAction = LEFT;
  motorStartTime = millis();
  Serial.println("Turning LEFT");
}

void turnRight() {
  moveNeckSmooth(0);
  analogWrite(RPWM_L, speedValue); analogWrite(LPWM_L, 0); // left forward
  analogWrite(RPWM_R, 0); analogWrite(LPWM_R, 0);          // stop right
  moving = true; currentAction = RIGHT;
  motorStartTime = millis();
  Serial.println("Turning RIGHT");
}

void stopMotors() {
  analogWrite(RPWM_L, 0); analogWrite(LPWM_L, 0);
  analogWrite(RPWM_R, 0); analogWrite(LPWM_R, 0);
  moving = false; currentAction = NONE;
  Serial.println("Motors STOPPED");
}


void setup() {
  Serial.begin(115200);
  while (!Serial);                
  Serial.setTimeout(100);

  pwm.begin();
  delay(300);                    
  pwm.setPWMFreq(50);

  // Initialize servos
  currentAngle[RIGHT_SHOULDER] = 0; pwm.setPWM(RIGHT_SHOULDER, 0, angleToPulse(0));
  currentAngle[RIGHT_ELBOW]    = 90; pwm.setPWM(RIGHT_ELBOW, 0, angleToPulse(90));
  currentAngle[LEFT_SHOULDER]  = 90; pwm.setPWM(LEFT_SHOULDER, 0, angleToPulse(90));
  currentAngle[LEFT_ELBOW]     = 90; pwm.setPWM(LEFT_ELBOW, 0, angleToPulse(90));

  neckAngle = 30; pwm.setPWM(NECK_SERVO, 0, angleToPulse(neckAngle));

  for (int i = 0; i < 16; i++) {
    if (i != RIGHT_SHOULDER && i != RIGHT_ELBOW && i != LEFT_SHOULDER && i != LEFT_ELBOW && i != NECK_SERVO) {
      currentAngle[i] = 90;
      pwm.setPWM(i, 0, angleToPulse(90));
    }
  }

  pinMode(RPWM_L, OUTPUT); pinMode(LPWM_L, OUTPUT);
  pinMode(RPWM_R, OUTPUT); pinMode(LPWM_R, OUTPUT);
  pinMode(REN_L, OUTPUT); pinMode(LEN_L, OUTPUT);
  pinMode(REN_R, OUTPUT); pinMode(LEN_R, OUTPUT);
  digitalWrite(REN_L, HIGH); digitalWrite(LEN_L, HIGH);
  digitalWrite(REN_R, HIGH); digitalWrite(LEN_R, HIGH);

  stopMotors();

  Serial.println("Robot Ready!");
  Serial.println("Commands: front, back, left, right, stop, rshake");
}


void loop() {
  if (Serial.available() > 0) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    if (cmd.length() == 0) return;

    if (cmd == "front") moveForward();
    else if (cmd == "back") moveBackward();
    else if (cmd == "left") turnLeft();
    else if (cmd == "right") turnRight();
    else if (cmd == "stop") stopMotors();
    else if (cmd == "rshake") rHandshake();
    else Serial.println("Unknown command");
  }

  if (moving) handsFront();

  // Stop motors automatically after 3 sec for turns
  if ((currentAction == LEFT || currentAction == RIGHT) &&
      (millis() - motorStartTime >= 3000)) {
    stopMotors();
    moveNeckSmooth(30);
    Serial.println("Turn complete: motors stopped, neck reset");
  }
}
