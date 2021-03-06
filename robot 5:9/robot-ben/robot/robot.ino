#include <Adafruit_TCS34725softi2c.h>

#include <RPLidar.h>
#include <Wire.h>
#include <Servo.h>
#include "Adafruit_TCS34725.h"
#include <SPI.h>
#include <Pixy.h>
#define redpin 3
#define greenpin 5
#define bluepin 6
#define commonAnode true
#define SDApin1 4
#define SCLpin1 5
#define SDApin2 6
#define SCLpin2 7
#define SDApin3 22
#define SCLpin3 24


//------------MOTOR SHIELD CONSTANTS------------------------
//B is right, A is left
int const A_FORWARD = LOW;
int const B_FORWARD = HIGH;
int const A_REVERSE = !A_FORWARD;
int const B_REVERSE = !B_FORWARD;
int const A_DIRECTION_PIN = 12;
int const A_BRAKE_PIN = 9;
int const A_SPEED_PIN = 3;
int const B_DIRECTION_PIN = 13;
int const B_BRAKE_PIN = 8;
int const B_SPEED_PIN = 11;
int const FULL_SPEED = 255;
int const FORWARD_SPEED = 180;
int const TURNING_SPEED = 180;
int const REVERSE_SPEED = 170;


double const END_TIME = 120000;

//-------------SERVO DECLARATIONS------------------------
Servo leftServo;
Servo rightServo;
Servo roller;
const int LEFT_SERVO_PIN = 46;
const int RIGHT_SERVO_PIN = 45;
const int ROLLER_PIN = 10;
//-------------SENSORS DECLARATIONS--------------------------------
Pixy pixy;
struct Sensor
{
  int pin;
  float value;

};
const int LEFT_LIGHT_PIN = A4;
const int RIGHT_LIGHT_PIN = A13;
const int LEFT_FRONT_IR_PIN = A7;
const int RIGHT_FRONT_IR_PIN = A15;
const int LEFT_REAR_IR_PIN = A5;
const int RIGHT_REAR_IR_PIN = A14;
const int LEFT_IR_PIN = A3;
const int RIGHT_IR_PIN = A12;
const int SONAR_PIN = A10;
const int VOLT_PIN = A11;

const int LOW_BATTERY_LEVEL = 925;



//QUAFFLE DETECTION CONSTS
int const TARGET_BUF = 30;
int const ALLOWABLE_BUF = 100;
//COLOR SIGNATURES
int const GREEN_SIG = 2;
int const YELLOW_SIG = 1;
int const LIGHT_SIG = 4;

//State variables
int quaffleDetectedRight;
int quaffleDetectedLeft;
int quaffleDetected;
int quaffleSeenThisLoop;
int quaffleX;
int quaffleY;
int loopsSinceQuaffleSeen = 1000;
int turnCounter = 0;

int collectingMode = 0;
int scoringMode = 0;
int followCenterLine = 1;
int followWall = 0;

int goalDetected = 0;
int goalX;
int goalY;
int goalSeenThisLoop = 0;
int goalCentered = 0;
int goalDetectedLeft;
int goalDetectedRight;

//color sensors
Adafruit_TCS34725softi2c rightRGB = Adafruit_TCS34725softi2c(TCS34725_INTEGRATIONTIME_2_4MS, TCS34725_GAIN_4X, SDApin1, SCLpin1);
Adafruit_TCS34725softi2c leftRGB = Adafruit_TCS34725softi2c(TCS34725_INTEGRATIONTIME_2_4MS, TCS34725_GAIN_4X, SDApin2, SCLpin2);
Adafruit_TCS34725softi2c rearRGB = Adafruit_TCS34725softi2c(TCS34725_INTEGRATIONTIME_2_4MS, TCS34725_GAIN_4X, SDApin3, SCLpin3);


void setup() {
  Serial.println("Starting");
  delay(2000);
  pinMode(LEFT_FRONT_IR_PIN, INPUT);
  pinMode(RIGHT_FRONT_IR_PIN, INPUT);
  pinMode(LEFT_REAR_IR_PIN, INPUT);
  pinMode(RIGHT_REAR_IR_PIN, INPUT);
  pinMode(LEFT_IR_PIN, INPUT);
  pinMode(RIGHT_IR_PIN, INPUT);
  pinMode(SONAR_PIN, INPUT);
  pinMode(LEFT_LIGHT_PIN, INPUT);
  pinMode(RIGHT_LIGHT_PIN, INPUT);
  pinMode(VOLT_PIN, INPUT);
  pinMode(A_DIRECTION_PIN, OUTPUT);
  pinMode(B_DIRECTION_PIN, OUTPUT);
  pinMode(A_BRAKE_PIN, OUTPUT);
  pinMode(B_BRAKE_PIN, OUTPUT);
  Serial.begin(9600);
  leftRGB.begin();
  rightRGB.begin();
  pixy.init();

#if defined(ARDUINO_ARCH_ESP32)
  ledcAttachPin(redpin, 1);
  ledcSetup(1, 12000, 8);
  ledcAttachPin(greenpin, 2);
  ledcSetup(2, 12000, 8);
  ledcAttachPin(bluepin, 3);
  ledcSetup(3, 12000, 8);
#else
  pinMode(redpin, OUTPUT);
  pinMode(greenpin, OUTPUT);
  pinMode(bluepin, OUTPUT);
#endif
  //forward(FORWARD_SPEED);
  quaffleDetectedRight = 0;
  quaffleDetectedLeft = 0;


  initializeServos();
  rollIn();

}

void loop() {
  uint16_t red, green, blue, clear;
  float leftHue, rightHue, s, v;



  while (millis() < END_TIME) {
    /*
        delay(200);
        float leftVal = getIrVal(LEFT_FRONT_IR_PIN);
        float rightVal = getIrVal(RIGHT_FRONT_IR_PIN);
        Serial.print(leftVal);
        Serial.print("  ");
        Serial.println(rightVal);
    */
    //Serial.println(millis());

    if (!quaffleSeenThisLoop) loopsSinceQuaffleSeen++;
    if (loopsSinceQuaffleSeen > 500) {
      stopRoll();

    }

    leftRGB.setInterrupt(false);
    rightRGB.setInterrupt(false);

    //generic loop counter
    static int i = 0;
    int j;
    quaffleSeenThisLoop = 0;

    //From pixy source---------------
    //pixy blocks
    uint16_t blocks;
    char buf[32];
    // grab blocks!
    blocks = pixy.getBlocks();
    //end pixy source-----------------


    //from HSV source ----------------
    leftRGB.getRawData(&red, &green, &blue, &clear);
    RGBtoHSV(red, green, blue, &leftHue, &s, &v);
    rightRGB.getRawData(&red, &green, &blue, &clear);
    RGBtoHSV(red, green, blue, &rightHue, &s, &v);

    //end HSV source ------------------
    //Serial.println(getVoltage());

    /*
        if (leftHue < 87 && leftHue > 77) {
          Serial.println("yellow detected left");
          reverse_right();
          turnCounter++;
        }
        if (rightHue < 90 && rightHue > 80) {
          Serial.println("yellow detected right");
          reverse_left();
          turnCounter++;
        }
    */
    /*
      if(getIrVal(LEFT_FRONT_IR_PIN)<8){
      Serial.println("Ir front right");
      reverse_left();
      turnCounter++;
      }

      if(getIrVal(RIGHT_FRONT_IR_PIN)<8){
      Serial.println("Ir front left");
      reverse_right();
      turnCounter++;
      }

      if(getIrVal(RIGHT_IR_PIN)<3){
      Serial.println("Ir right");
      turn_left();
      }
       if(getIrVal(LEFT_IR_PIN)<3){
      Serial.println("Ir left");
      turn_right();
      }
    */
    /*

      if(getSonarVal(SONAR_PIN) < 0){
      Serial.println("sonar pin");
      }

      //rear sensors
      if(getIrVal(RIGHT_REAR_IR_PIN)<6){
      Serial.println("Ir rear right");
      }
      if(getIrVal(LEFT_REAR_IR_PIN)<6){
      Serial.println("Ir rear left");
      }
    */
    while (followCenterLine) {
      Serial.println("following line");
      leftRGB.getRawData(&red, &green, &blue, &clear);
      RGBtoHSV(red, green, blue, &leftHue, &s, &v);
      // Serial.println(leftHue);
      int noAction = 1;

      if (353 < leftHue && leftHue < 363 && noAction) {
        Serial.println("red");
        forward(FORWARD_SPEED);
        noAction = 0;
      }
      if (192 < leftHue && leftHue < 202 && noAction) {
        Serial.println("blue");
        left(TURNING_SPEED);
        noAction = 0;
      }
      if (150 < leftHue && leftHue < 165 && noAction) {
        Serial.println("gray");
        right(TURNING_SPEED);
        noAction = 0;
      }
      if (getIrVal(LEFT_FRONT_IR_PIN) < 9) {
        Serial.println("Wall detected");
        followCenterLine = 0;
        followWall = 1;
        brake();
        Serial.print(getIrVal(LEFT_FRONT_IR_PIN));
        Serial.print(" ");
        Serial.println(getIrVal(RIGHT_FRONT_IR_PIN));
        Serial.println(abs(getIrVal(LEFT_FRONT_IR_PIN) - getIrVal(RIGHT_FRONT_IR_PIN)));

        lineUpWall();


        Serial.print(getIrVal(LEFT_FRONT_IR_PIN));
        Serial.print(" ");
        Serial.println(getIrVal(RIGHT_FRONT_IR_PIN));
        //turn right until nothing in front
        while (getIrVal(LEFT_FRONT_IR_PIN) < 10 || getIrVal(RIGHT_FRONT_IR_PIN) < 10) {
          right(TURNING_SPEED);
        }
        Serial.println("done ir turn");
        while(!(leftHue <87 && leftHue > 77)){
          leftRGB.getRawData(&red, &green, &blue, &clear);
      RGBtoHSV(red, green, blue, &leftHue, &s, &v);
          right(TURNING_SPEED);
        }
        Serial.println("done no yellow turn");
        while(leftHue <87 && leftHue > 77){
          leftRGB.getRawData(&red, &green, &blue, &clear);
      RGBtoHSV(red, green, blue, &leftHue, &s, &v);
          right(TURNING_SPEED);
        }
        Serial.println("done yellow turn");
        brake();

      }

    }

    while (followWall) {
      leftRGB.getRawData(&red, &green, &blue, &clear);
      RGBtoHSV(red, green, blue, &leftHue, &s, &v);
      float wallDist = getIrVal(LEFT_IR_PIN);
      Serial.println(wallDist);
      if (wallDist < 8||(leftHue<87 && leftHue>77)) {
        Serial.println("too close to wall");
        forwardRight(FORWARD_SPEED);
      }
      
      else {
        if (wallDist > 9) {
          forwardLeft(FORWARD_SPEED);
        }
        else {
          forward(FORWARD_SPEED);
        }
      }
      if (getIrVal(RIGHT_FRONT_IR_PIN) < 9) {
        Serial.println("found goal");
        Serial.println(wallDist);
        Serial.println(getIrVal(LEFT_FRONT_IR_PIN));
        Serial.println(getIrVal(RIGHT_FRONT_IR_PIN));
        followWall = 0;
        slowLineUpWall();
        delay(1000);
        raiseArm();
        forward(FORWARD_SPEED);
        delay(400);
        brake();
        rollOut();
        delay(1000);
        reverse(FORWARD_SPEED);
        delay(500);
        brake();
        lowerArm();
      }
    }



    if (analogRead(VOLT_PIN) < LOW_BATTERY_LEVEL) {
      Serial.println("LOW BATTERY");
    }


    if (getLightVal(LEFT_LIGHT_PIN) > 300) {
      float x = 0;
      const int q = 100;
      for (int i = 0; i < q; i++) {
        x += getLightVal(LEFT_LIGHT_PIN);
      }
      x = x / q;
      Serial.print("Light left ");
      Serial.println(x);
      delay(400);
    }
    if (getLightVal(RIGHT_LIGHT_PIN) > 300) {
      float x = 0;
      const int q = 100;
      for (int i = 0; i < q; i++) {
        x += getLightVal(RIGHT_LIGHT_PIN);
      }
      x = x / q;
      Serial.print("Light right ");
      Serial.println(x);
      delay(400);
    }
    //from pixy source ----------------
    if (blocks)
    {
      i++;

      // do this (print) every 50 frames because printing every
      // frame would bog down the Arduino
      if (i % 1 == 0)
      {
        sprintf(buf, "Detected %d:\n", blocks);
        //Serial.print(buf);
        for (j = 0; j < blocks; j++)
        {
          //end from pixy source--------------
          /*
            sprintf(buf, "  block %d: ", j);
            Serial.print(buf);
            pixy.blocks[j].print();*/

          if (pixy.blocks[j].signature == GREEN_SIG && collectingMode) {
            rollIn();
            loopsSinceQuaffleSeen = 0;

            if (!quaffleDetected) {
              Serial.print("Green ball ");
              Serial.print(millis());
              Serial.print(" ");
              Serial.println(i);

            }

            quaffleSeenThisLoop = 1;
            quaffleDetected = 1;
            quaffleX = pixy.blocks[j].x;
            quaffleY = pixy.blocks[j].y;
            //if quaffle is within buffer begin turning

            if (quaffleX > (155 + ALLOWABLE_BUF) && quaffleDetectedRight == 0) {
              forwardRight(TURNING_SPEED);
              Serial.println("Quaffle detected to the right, turning");
              quaffleDetectedRight = 1;
            }
            if (quaffleX < (155 - ALLOWABLE_BUF) && quaffleDetectedLeft == 0 && !quaffleDetectedRight) {
              forwardLeft(TURNING_SPEED);
              Serial.println("Quaffle detected to the left, turning");
              quaffleDetectedLeft = 1;
            }



            if (quaffleX < 155 + TARGET_BUF && quaffleDetectedRight == 1 ) {
              forward(FORWARD_SPEED);
              quaffleDetectedRight = 0;
              Serial.println("Quaffle centered");
            }
            if (quaffleX > 155 - TARGET_BUF && quaffleDetectedLeft == 1) {
              forward(FORWARD_SPEED);
              quaffleDetectedLeft = 0;
              Serial.println("Quaffle centered");
            }


          }
          if (pixy.blocks[j].signature == LIGHT_SIG && pixy.blocks[j].width * pixy.blocks[j].height > 50 && scoringMode
              && pixy.blocks[j].height < 160) {
            goalDetected = 1;
            goalSeenThisLoop = 1;
            goalX = pixy.blocks[j].x;
            goalY = pixy.blocks[j].y;

            if (goalX > (155 + ALLOWABLE_BUF) && goalDetectedRight == 0) {
              forwardRight(TURNING_SPEED);
              Serial.println("Goal detected to the right, turning");
              goalDetectedRight = 1;
            }
            if (goalX < (155 - ALLOWABLE_BUF) && goalDetectedLeft == 0 && !goalDetectedRight) {
              forwardLeft(TURNING_SPEED);
              Serial.println("Goal detected to the left, turning");
              goalDetectedLeft = 1;
            }



            if (goalX < 155 + TARGET_BUF && goalDetectedRight == 1 ) {
              forward(FORWARD_SPEED);
              goalDetectedRight = 0;
              Serial.println("Goal centered");
            }
            if (goalX > 155 - TARGET_BUF && goalDetectedLeft == 1) {
              forward(FORWARD_SPEED);
              goalDetectedLeft = 0;
              Serial.println("Goal centered");
            }

          }

        }

      }
    }
    if (loopsSinceQuaffleSeen > 10 && quaffleDetected) {
      Serial.print("Ball disappeared ");
      Serial.print(millis());
      Serial.print(" ");
      Serial.println(i);
      quaffleDetectedRight = 0;
      quaffleDetectedLeft = 0;
      quaffleDetected = 0;
      forward(FORWARD_SPEED);
    }
    if (goalDetected && (getIrVal(LEFT_FRONT_IR_PIN) < 7 || getIrVal(RIGHT_FRONT_IR_PIN) < 7) && scoringMode) {
      Serial.println("at goal!");
      excited();
      while (1) {
        brake();
      }
    }
    if (!goalSeenThisLoop && goalDetected) {
      Serial.print("Goal disappeared ");
      Serial.print(millis());
      Serial.print(" ");
      Serial.println(i);
      goalDetectedRight = 0;
      goalDetectedLeft = 0;
      goalDetected = 0;
      forward(FORWARD_SPEED);
    }

  }
  brake();
  stopRoll();
}

//----------------NAV FUNC=----------
void slowLineUpWall() {
  while (abs(getIrVal(LEFT_FRONT_IR_PIN) - getIrVal(RIGHT_FRONT_IR_PIN)) > .1) {
    Serial.print(getIrVal(LEFT_FRONT_IR_PIN));
    Serial.print(" ");
    Serial.println(getIrVal(RIGHT_FRONT_IR_PIN));

    while (getIrVal(LEFT_FRONT_IR_PIN) < getIrVal(RIGHT_FRONT_IR_PIN)) {
      Serial.println("rotating left");
      left(TURNING_SPEED);
      delay(30);
      brake();
    }
    
    while (getIrVal(RIGHT_FRONT_IR_PIN) < getIrVal(LEFT_FRONT_IR_PIN)) {
      Serial.println("rotating right");
      right(TURNING_SPEED);
      delay(30);
      brake();
    }
    
  }
}
void lineUpWall() {
  while (abs(getIrVal(LEFT_FRONT_IR_PIN) - getIrVal(RIGHT_FRONT_IR_PIN)) > .1) {
    Serial.print(getIrVal(LEFT_FRONT_IR_PIN));
    Serial.print(" ");
    Serial.println(getIrVal(RIGHT_FRONT_IR_PIN));

    while (getIrVal(LEFT_FRONT_IR_PIN) < getIrVal(RIGHT_FRONT_IR_PIN)) {
      Serial.println("rotating left");
      left(TURNING_SPEED);
    }
    
    
    while (getIrVal(RIGHT_FRONT_IR_PIN) < getIrVal(LEFT_FRONT_IR_PIN)) {
      Serial.println("rotating right");
      right(TURNING_SPEED);
      
    }
    
  }
}

//-----------------SENSOR FUNCTIONS--------------------
float getIrVal(int pin) {
  //return sensor.value * sensor.modifier;
  // return 37-sensor.value;
  return 13 * pow(analogRead(pin) * 0.0048828125, -1);
}
float getSonarVal(int pin) {
  return analogRead(pin) / 5;
}
float getLightVal(int pin) {
  return pow(10, analogRead(pin) * 5.0 / 1024);
}
float getVoltage() {
  return analogRead(VOLT_PIN) / 121.79;
}



/*
  int updateSensors() {
  for (int i = 0; i < SENSOR_QUANT; i++) {
    //Serial.print("Update Sensors ");
    //Serial.println(analogRead(sensors[i].pin));
    sensors[i].value = analogRead(sensors[i].pin);
    //Serial.print("updateSensors val: ");
    //Serial.println(sensors[i].value);
  }
  }
  /*
  int greenBlockDetected( blocks){
  int blockDetected = 0;
  for(int i=0;i<blocks;i++){
    if(blocks[i].signature == 2) blockDetected = 1;
  }
  }
*/

//SOURCES:
//PIXY source:
// begin license header
//
// This file is part of Pixy CMUcam5 or "Pixy" for short
//
// All Pixy source code is provided under the terms of the
// GNU General Public License v2 (http://www.gnu.org/licenses/gpl-2.0.html).
// Those wishing to use Pixy source code, software and/or
// technologies under different licensing terms should contact us at
// cmucam@cs.cmu.edu. Such licensing terms are available for
// all portions of the Pixy codebase presented here.
//
// end license header
//HSV source:
// https://www.cs.rit.edu/~ncs/color/t_convert.html
