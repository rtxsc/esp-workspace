#include <Encoder.h>
#define DELAY_MICROSEC 500
//#define ENCODER_CONTROL
#define INPUT_CONTROL
int incomingInt = 0; // for incoming serial data

// Stepper Motor X
  Encoder myEnc(12, 13);
  const int stepPin = 2; //X.STEP
  const int dirPin = 5; // X.DIR
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
        Serial.println(":Please insert any value ranging from 0-60 (cm) into the field:");

  #endif
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

  while (Serial.available() > 0) {
      Serial.println(":::Manual Input Controlled Linear Stepper:::");
      // read the incoming byte:
      incomingInt = Serial.parseInt(); ;
      Serial.print("I received: ");
      Serial.print(incomingInt, DEC);
      Serial.println(" cm");
      int newPosition = map(incomingInt, 0, 60, 0, 3000);      
      Serial.print("manual newPosition: ");
      Serial.println(newPosition, DEC);
      
      if (newPosition != oldPosition && newPosition > oldPosition) {
    digitalWrite(dirPin,HIGH); // Enables the motor to move in a particular direction
    oldPosition = newPosition;
     for(x = oldPosition+(x-newPosition); x <= newPosition; x++) {
       digitalWrite(stepPin,HIGH); 
       delayMicroseconds(DELAY_MICROSEC); 
       digitalWrite(stepPin,LOW); 
       delayMicroseconds(DELAY_MICROSEC); 
     }

  }

  Serial.println("\t:::::::::::Scanning temp:::::::::::");
  delay(250);
  Serial.println("\t:::::::::::Scanning temp DONE:::::::::::");
  delay(250);
  Serial.println("\t:::::::::::Going Home:::::::::::");
  delay(250);
  newPosition = 0; // return back after 5 seconds

  if (newPosition != oldPosition && newPosition < oldPosition) {
    digitalWrite(dirPin,LOW); // Enables the motor to move in a particular direction
    oldPosition = newPosition;
     for(x = oldPosition+(x-newPosition); x >= newPosition; --x) {
       digitalWrite(stepPin,HIGH); 
       delayMicroseconds(DELAY_MICROSEC); 
       digitalWrite(stepPin,LOW); 
       delayMicroseconds(DELAY_MICROSEC); 
     }
  }
  Serial.println(":Please insert any value ranging from 0-60 (cm) into the field:");

      
  }
  delay(2000); // master delay in this USER INPUT loop

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
