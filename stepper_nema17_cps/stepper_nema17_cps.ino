#include "GyverTM1637.h" // for BIG VERSION

#define CLK Y_STEP
#define DIO Z_STEP
#include "cnc_shield_uno.h"
#include <Encoder.h>
#define DELAY_MICROSEC 500
//#define ENCODER_CONTROL
#define INPUT_CONTROL
int height_cm = 0; // for incoming serial data

GyverTM1637 disp(CLK, DIO);

// Stepper Motor X
Encoder myEnc(SpnEn, SpnDir);
const int stepPin = X_STEP; //X.STEP
const int dirPin = X_DIR; // X.DIR
long oldPosition  = -999;
int x;

void setup() {
  // Sets the two pins as Outputs
  Serial.begin(115200);
  pinMode(stepPin,OUTPUT); 
  pinMode(dirPin,OUTPUT);
  Serial.print("DELAY_MICROSECOND:");
  Serial.println(DELAY_MICROSEC);

  #ifdef INPUT_CONTROL
        Serial.println(":::Manual Input Controlled Linear Stepper:::");
        Serial.println(":Please insert any value ranging from 140-200(cm) into the field:");
  #endif

  disp.clear();
  disp.brightness(7);  // яркость, 0 - 7 (минимум - максимум)
  runningText();
  randomSeed(analogRead(Abort));


 }
 void loop() {

  #ifdef ENCODER_CONTROL
  Serial.println(":::Encoder Controlled Linear Stepper:::");

  long newPosition = myEnc.read() / 2;
  if (newPosition != oldPosition && newPosition > oldPosition) {
    digitalWrite(dirPin,HIGH); // Enables the motor to move in a particular direction
    oldPosition = newPosition;
     // Makes 200 pulses for making one full cycle rotation
     for(x = oldPosition+(x-newPosition); x <= newPosition*200; x++) {
       digitalWrite(stepPin,HIGH); 
       delayMicroseconds(DELAY_MICROSEC); 
       digitalWrite(stepPin,LOW); 
       delayMicroseconds(DELAY_MICROSEC); 
     }
  }

  if (newPosition != oldPosition && newPosition < oldPosition) {
    digitalWrite(dirPin,LOW); // Enables the motor to move in a particular direction
    oldPosition = newPosition;
     // Makes 200 pulses for making one full cycle rotation
     for(x = oldPosition+(x-newPosition); x >= newPosition*200; --x) {
       digitalWrite(stepPin,HIGH); 
       delayMicroseconds(DELAY_MICROSEC); 
       digitalWrite(stepPin,LOW); 
       delayMicroseconds(DELAY_MICROSEC); 
     }
  }

  #elif defined INPUT_CONTROL

    byte here[4] = {_h, _e, _r, _e};
    disp.point(0);   // выкл/выкл точки
    disp.twistByte(here, 25);

    while (Serial.available() > 0) {
        Serial.println(":::Manual Input Controlled Linear Stepper:::");
        // read the incoming byte:
        height_cm = Serial.parseInt(); 
        if(height_cm < 140) height_cm = 140;
        if(height_cm > 200) height_cm = 200;

        Serial.print("[DUMMY ULTRASONIC] Height received: ");
        Serial.print(height_cm, DEC);
        Serial.println(" cm");
        int mapped_height = map(height_cm, 140, 200, 0, 60);
        int newPosition = map(mapped_height, 0, 60, 0, 3000); 

        byte height[4];
        for (int i = 3 ; i >= 0 ; i--)
        {
          height[i] = height_cm % 10 ;
          height_cm /= 10 ;
        }
        disp.point(1);    // выкл/выкл точки
        height[0] = _h;
        disp.clear();
        disp.twistByte(height, 1);     // скорость прокрутки 100
        disp.scroll(height, 100);     // скорость прокрутки 100

        delay(1000);

        Serial.print("Moving to newPosition at: ");
        Serial.println(newPosition, DEC);
        
        if (newPosition != oldPosition && newPosition > oldPosition) {
          byte hold[4] = {_h, _o, _l, _d};
          disp.point(0);   // выкл/выкл точки
          disp.twistByte(hold, 25);

      digitalWrite(dirPin,HIGH); 
      oldPosition = newPosition;
      for(x = oldPosition+(x-newPosition); x <= newPosition; x++) {
        digitalWrite(stepPin,HIGH); 
        delayMicroseconds(DELAY_MICROSEC); 
        digitalWrite(stepPin,LOW); 
        delayMicroseconds(DELAY_MICROSEC); 
      }

    }
    disp.clear();
    Serial.println("\t:::::::::::Scanning temp:::::::::::");
    byte tempc[4];
    int tempc_int = random(3500,4000);
    for (int i = 3 ; i >= 0 ; i--)
    {
      tempc[i] = tempc_int % 10;
      tempc_int /= 10 ;
    }
    tempc[3] = '\0';
    disp.clear();
    disp.twistByte(tempc, 1);     // скорость прокрутки 100
    disp.point(1);   // выкл/выкл точки
    disp.scroll(tempc, 100);     // скорость прокрутки 100
    delay(1000);

    Serial.println("\t:::::::::::Scanning temp DONE:::::::::::");
    Serial.println("\t:::::::::::Going Home:::::::::::");
    delay(250);
    newPosition = 0; // return back after 5 seconds

    if (newPosition != oldPosition && newPosition < oldPosition) {
      digitalWrite(dirPin,LOW); 
      oldPosition = newPosition;
      for(x = oldPosition+(x-newPosition); x >= newPosition; --x) {
        digitalWrite(stepPin,HIGH); 
        delayMicroseconds(DELAY_MICROSEC); 
        digitalWrite(stepPin,LOW); 
        delayMicroseconds(DELAY_MICROSEC); 
      }
    }

    byte done[4] = {_d, _o, _n, _e};
    disp.point(0);   // выкл/выкл точки
    disp.twistByte(done, 25);
    Serial.println(":Please insert a value ranging from 140-200 (cm) into the field:"); 
    }
    delay(1000); // master delay in this USER INPUT loop
    disp.clear();

  #else
    Serial.println(":::Automated Linear Stepper:::");
    digitalWrite(dirPin,HIGH); //Changes the rotations direction
    for(int x = 0; x < 3000; x++) {
      digitalWrite(stepPin,HIGH);
      delayMicroseconds(DELAY_MICROSEC);
      digitalWrite(stepPin,LOW);
      delayMicroseconds(DELAY_MICROSEC);
    }
    delay(1000); // One second delay
    
    digitalWrite(dirPin,LOW); //Changes the rotations direction
    for(int x = 0; x < 3000; x++) {
      digitalWrite(stepPin,HIGH);
      delayMicroseconds(DELAY_MICROSEC);
      digitalWrite(stepPin,LOW);
      delayMicroseconds(DELAY_MICROSEC);
    }
    delay(1000);
 #endif
 }

void runningText() {
  byte welcome_banner[] = {_H, _E, _L, _L, _O, _empty,        
                          };
  disp.runningString(welcome_banner, sizeof(welcome_banner), 100);  // 200 это время в миллисекундах!
}