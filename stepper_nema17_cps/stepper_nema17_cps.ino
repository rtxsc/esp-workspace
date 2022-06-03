#include "GyverTM1637.h" // for BIG VERSION
#include <HCSR04.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_MLX90614.h>

#define SCREEN_WIDTH 128 
#define SCREEN_HEIGHT 64 
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define CLK Y_STEP
#define DIO Z_STEP

#include "cnc_shield_uno.h"
#include <Encoder.h>
#define DELAY_MICROSEC 500
//#define ENCODER_CONTROL
#define INPUT_CONTROL
int height_cm = 0; // for incoming serial data
byte height_sense[4];
int distanceInt;

GyverTM1637               disp(CLK, DIO);
UltraSonicDistanceSensor  distanceSensor(Y_DIR, Z_DIR);  // Initialize sensor that uses digital pins 13 and 12.
Adafruit_MLX90614 mlx = Adafruit_MLX90614();

// Stepper Motor X
Encoder myEnc(SpnEn, SpnDir);
const int stepPin = X_STEP; //X.STEP
const int dirPin = X_DIR; // X.DIR
long oldPosition  = -999;
int x;

double new_emissivity = 0.68;

void setup() {

  Serial.begin(115200);
   mlx.begin(); 
  // read current emissivity
  Serial.print("Current emissivity = "); Serial.println(mlx.readEmissivity());

  // set new emissivity
  Serial.print("Setting emissivity = "); Serial.println(new_emissivity);
  mlx.writeEmissivity(new_emissivity); // this does the 0x0000 erase write

  // read back
  Serial.print("New emissivity = "); Serial.println(mlx.readEmissivity());  // if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3D for 128x64
  //   Serial.println(F("SSD1306 allocation failed"));
  //   for(;;); // Don't proceed, loop forever
  // }
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

    int distanceToObject = distanceSensor.measureDistanceCm();
    // Serial.print("float dist:");
    // Serial.println(distanceToObject);

    // display.clearDisplay();
    // display.setTextSize(2);             // Normal 1:1 pixel scale
    // display.setTextColor(SSD1306_WHITE);        // Draw white text
    // display.setCursor(0,0);             // Start at top-left corner
    // display.print("distance:");
    // display.setCursor(0,16);             // Start at top-left corner
    // display.println(distance);
    // display.display();

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

    if(millis() % 2 == 0){
      byte here[4] = {_h, _e, _r, _e};
      disp.point(0);   // выкл/выкл точки
      disp.twistByte(here, 25);
    }
    else{
      disp.displayInt(distanceToObject);
    }
  

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

    int head_temp = mlx.readObjectTempC();
    Serial.print("forehead temp:");
    Serial.println(head_temp);

    byte tempc[4];
    int tempc_int = head_temp*100; // random(3500,4000);
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