/*
 Prototype for Mini Launcher, does Pan and Power calculations
 Uses PID for motor control of Pan, and has user input
 */
#include <math.h>
#include <Servo.h>
//DC Motor Initialization
#include <Wire.h>
#include <Adafruit_MotorShield.h>
//#include "utility/Adafruit_PWMServoDriver.h"

Adafruit_MotorShield AFMS = Adafruit_MotorShield();

#define limitSwitchPin 1
//Servo
Servo latch1; //70 locked 110 unlocked
Servo latch2; //100 locked 55 unlocked

//Pan Motor
Adafruit_DCMotor *panMotor = AFMS.getMotor(3);

//Winch Motors
Adafruit_DCMotor *winch1 = AFMS.getMotor(1);
Adafruit_DCMotor *winch2 = AFMS.getMotor(2);
Adafruit_DCMotor *winch3 = AFMS.getMotor(4);

//PAN VARIABLES
int panPot = A0;
int panTopSpeed = 100;
float panEncoderValue;    //IN DEGREES
float panOffset = 94.9 + 20; //IN DEGREES
float panError = 0;  //IN DEGREES
float panPGain = 10;
float panDGain = 20;
float panDerivative = 0;
float panProportional = 0;
int panDt = 50;
int panTarget = 0;
int panMotorSpeed = 0;
int panThreshold = 2;
int prevTime = 0;

//used in String parsing input
int separator;
String string1;
String string2;
String ID;
String stringvalue;
int value;
int len;
int Distance;

//ARM VARIABLES
int armPot = A1;
float armEncoderValue;
float armOffset = 41
; //IN DEGREES
bool reset = false;
bool fire = false;
bool arm = false;
int armTarget = 0;

String input;


void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial.println("starting");
  input = "default";

  //sets up motors
  AFMS.begin();
  
  latch1.attach(9);
  latch2.attach(10);
  fireLauncher();
  panMotor->run(RELEASE);

  while(!reset){
    getSensorData();
    resetLauncher();
  }
  Serial.println("reset");

}


//main loop
void loop() {
  Serial.setTimeout(50);
  getSensorData();
  if (Serial.available()) {
    getInput();
  }
  pan();

  if(reset && arm){
    armLauncher();
  }
  if(fire && !arm){
    fireLauncher();
    panMotor->run(RELEASE);
    delay(500);
    reset = false;
    fire = false;
  }
  if(!reset){
    panTarget = 0;
    resetLauncher();
  }
}


//reads current values of encoders for control
void getSensorData() {

  panEncoderValue = voltageToDegrees(analogRead(panPot)) - panOffset;
  //Serial.println(panEncoderValue);
  armEncoderValue = voltageToDegrees(analogRead(armPot)) - armOffset;
  Serial.println(armEncoderValue);
}


//conversion function
float voltageToDegrees(float voltage) {
  // turns raw pot data into degrees
  return (voltage * 220) / 1024;
}



//Pan Motor moves to set angle point using PID control
void pan() {
  //Turns target and current encoder value to power output
  if (panTarget > 80) {
    panTarget = 80;
  }
  if (panTarget < -80) {
    panTarget = 80;
  }
  if (millis() - prevTime > panDt) {
    int prevPanEncoderValue = panEncoderValue;
    getSensorData();
    panDerivative = panDGain * (panEncoderValue - prevPanEncoderValue);
    prevTime = millis();
  }
  panError = float(panTarget) - panEncoderValue;
  //Proportional Term of PID
  panProportional = panPGain *  panError;
  //Calculate new motor speed
  panMotorSpeed = panProportional + panDerivative;

  if (panMotorSpeed > panTopSpeed) {
    panMotorSpeed = panTopSpeed;
  }
  if (panMotorSpeed < -panTopSpeed) {
    panMotorSpeed = -panTopSpeed;
  }
  if (abs(panError) < abs(panThreshold)) {
    panMotorSpeed = 0;
  }
  //Serial.println(panError);
  turnpanMotor(panMotorSpeed);
}


//Function for turning the Pan Motor
void turnpanMotor(int motorSpeed) {
  //Rotates the motors
  if (motorSpeed > 0) {
    panMotor->setSpeed(motorSpeed);
    panMotor->run(FORWARD);
  } else if (motorSpeed < 0) {
    panMotor->setSpeed(abs(motorSpeed));
    panMotor->run(BACKWARD);
  } else {
    panMotor ->run(RELEASE);
  }
}


//Function for turning Arm Motors
void turnArmMotors(int motorSpeed) {
  if (motorSpeed > 0) {
    winch1->setSpeed(motorSpeed);
    winch2->setSpeed(motorSpeed);
    winch3->setSpeed(motorSpeed);
    winch1->run(FORWARD);
    winch2->run(FORWARD);
    winch3->run(FORWARD);
  } else if (motorSpeed < 0) {
    winch1->setSpeed(abs(motorSpeed));
    winch2->setSpeed(abs(motorSpeed));
    winch3->setSpeed(motorSpeed);
    winch1->run(BACKWARD);
    winch2->run(BACKWARD);
    winch3->run(BACKWARD);
  } else {
    winch1->run(RELEASE);
    winch2->run(RELEASE);
    winch3->run(RELEASE);
  }
}



//Manual input for testing and human control
void getInput() {
    //INPUT IN FORMAT a = 20, power = 30
    input = Serial.readString();
    if(input == "fire"){
      fire = true;
      Serial.println("FIRING!");
    }
    if(input == "reset"){
      fire = true;
      Serial.println("Resetting launcher");
    }
    len = input.length()+1;
    separator = input.indexOf(",");
    
    if (separator != -1){
      string1 = input.substring(0, separator);
      string2 = input.substring(separator+1, len);
      string1.trim();
      string2.trim();
      
      separator = string1.indexOf("=");
      len = string1.length()+1;

      ID = string1.substring(0, separator);
      stringvalue = string1.substring(separator+1,len);
      stringvalue.trim();
      value = stringvalue.toInt();
      ID.trim();
      if(ID == "a"){
        panTarget = value;
        Serial.print("Angle set to: ");
        Serial.println(panTarget);
      }
      
      separator = string2.indexOf("=");
      len = string2.length()+1;
      ID = string2.substring(0, separator);
      stringvalue = string2.substring(separator+1,len);
      stringvalue.trim();
      value = stringvalue.toInt();
      ID.trim();
      if(ID == "power"){
        armTarget = value;
        if(armTarget > 0){
          arm =  true;
          Serial.print("Power set to: ");
          Serial.println(armTarget);
        }else{
          Serial.println("Negative Power Given, try agan");
        }
      }
    }   

}


//Arm motors move back to set distance using essentially bang bang control
void armLauncher() {
  if(armTarget > 50){
    armTarget = 50;
  }
  if(armEncoderValue < armTarget){
    turnArmMotors(100);
  }
  else{
    fire = true;
    arm = false;
    turnArmMotors(10);
  }
}



//Re-arms the launcher using essentially bang bang control
void resetLauncher() {
  if (armEncoderValue > 0){
    turnArmMotors(-60);
  }else{
    turnArmMotors(0);
    reset = true;
    activateLatch();
    delay(100);
  }
}


//Arm Functions
void activateLatch() {
  latch2.detach();
  latch1.detach();
  //latch2.write(100);
  //latch1.write(70);
}
void fireLauncher() {
  //latch2.write(55);
  //latch1.write(110);
}

