#include "GyverTM1637.h" // for BIG VERSION
#include <HCSR04.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_MLX90614.h>
#include "Ultrasonic.h"
#include <EEPROM.h>
/*
Pin usage CPS-HPV
D2 - A4988 X.STEP
D3 - Gyver CLK
D4 - Gyver CLK
D5 - A4988 X.DIR
D6 - HCSR04 TRIG
D7 - HCSR04 ECHO
D8
D9 - GROVE PRESENCE HCSR04
D10 - GROVE PRESENCE HCSR04 LIGHT SIGNAL
D11
D12 - Encoder SpnEn
D13 - Encoder SpnDir
A0
A1
A2
A3
A4
A5

*/
#define MAX_STEPS           3090      
#define STEPPER_POS_ADDRESS 0
#define height_offset       12

#define SCREEN_WIDTH 128 
#define SCREEN_HEIGHT 64 
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)

#define CLK Y_STEP
#define DIO Z_STEP
#define PRESENCE_LED Y_POS_NEG
#define SENSOR_HEIGHT 200
#define MODE_SELECT Y_POS_NEG

#include "cnc_shield_uno.h"
#include <Encoder.h>
#define DELAY_MICROSEC 500
//#define ENCODER_CONTROL
#define INPUT_CONTROL

byte height_sense[4];
bool presence_detected = false;

GyverTM1637               disp(CLK, DIO);
UltraSonicDistanceSensor  distanceSensor(Y_DIR, Z_DIR);  // normal HCSR04 - Height (could be changed to Grove)
Ultrasonic                ultrasonic(X_POS_NEG); // Grove HCSR04 - Presence
Adafruit_MLX90614         mlx = Adafruit_MLX90614();
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Stepper Motor X
Encoder myEnc(SpnEn, SpnDir);
const uint8_t stepPin = X_STEP; //X.STEP
const uint8_t dirPin = X_DIR; // X.DIR
int oldPosition  = -999;
int newPosition;

// int eeprom_val; //  disabled for memory reservation
uint16_t DEFAULT_POS;

// #define CHANGE_EMISSIVITY

#ifdef CHANGE_EMISSIVITY
  double new_emissivity = 0.5;
#endif

int readIntFromEEPROM(int address)
{
  return (EEPROM.read(address) << 8) + EEPROM.read(address + 1);
}

void writeIntIntoEEPROM(int address, int number)
{ 
  EEPROM.write(address, number >> 8);
  EEPROM.write(address + 1, number & 0xFF);
}

void setup() {
  Serial.begin(115200);
  mlx.begin(); 

  pinMode(MODE_SELECT, INPUT_PULLUP);

  // for(int addr = 0; addr < 5; addr++){
  //   eeprom_val = EEPROM.read(addr);
  //   Serial.print("Memory ");
  //   Serial.print(addr);
  //   Serial.print("\t");
	//   Serial.print(eeprom_val);
	//   Serial.println();
  //   delay(100);
  // }

  oldPosition = readIntFromEEPROM(STEPPER_POS_ADDRESS);
  if(oldPosition==255){
      Serial.println(":::Stepper Default Position Not Saved in Memory:::");
      delay(1000);
  }

  #ifdef CHANGE_EMISSIVITY
    // read current emissivity
    Serial.print("Current emissivity = "); Serial.println(mlx.readEmissivity());
    // set new emissivity
    Serial.print("Setting emissivity = "); Serial.println(new_emissivity);
    mlx.writeEmissivity(new_emissivity); // this does the 0x0000 erase write
    // read back
    Serial.print("New emissivity = "); Serial.println(mlx.readEmissivity());  // if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3D for 128x64
  #endif
  pinMode(stepPin,OUTPUT); 
  pinMode(dirPin,OUTPUT);
  pinMode(PRESENCE_LED,OUTPUT); // LED at D10 as presence status
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

  Serial.println("\n\n::::::::: Mode :::::::::");
  Serial.print("\t");
  Serial.println(digitalRead(MODE_SELECT));
  Serial.println("::::::::: Mode :::::::::");

  if(digitalRead(MODE_SELECT) == 1){
    DEFAULT_POS = TOP_POS; // either BOTTOM_POS = 0 or TOP_POS = MAX_STEPS
    running_TOP_POS();
  }
  else{
    DEFAULT_POS = BOTTOM_POS; // either BOTTOM_POS = 0 or TOP_POS = MAX_STEPS
    running_BOTTOM_POS();
  }

  disp.clear();
  Serial.print("[oldPosition] loaded from memory:");
  Serial.println(oldPosition);
  if(DEFAULT_POS == TOP_POS && oldPosition != DEFAULT_POS){
    newPosition = TOP_POS;
    writeIntIntoEEPROM(STEPPER_POS_ADDRESS,DEFAULT_POS);
    Serial.print(" >>>>>>>>>>>>>>> TOP_POS [newPosition] written memory:");
    Serial.println(newPosition);

    if (newPosition != oldPosition && newPosition > oldPosition) {
        Serial.println("Driving stepper to Top Position");
        byte retn[4] = {_r, _e, _t, _n};
        disp.point(0);   
        disp.twistByte(retn, 25);

        digitalWrite(dirPin,HIGH);
        for(int x_init = oldPosition; x_init <= newPosition; x_init++) {
          // Serial.print("x_init_up:");
          // Serial.println(x_init);
          digitalWrite(stepPin,HIGH); 
          delayMicroseconds(DELAY_MICROSEC); 
          digitalWrite(stepPin,LOW); 
          delayMicroseconds(DELAY_MICROSEC); 
        }
        oldPosition = newPosition; // Must be done after travel completion (after for-loop)
        Serial.println("\t::::::::::: Sensor Idling at TOP_POS :::::::::::");

     }
  }

  if(DEFAULT_POS == BOTTOM_POS && oldPosition != DEFAULT_POS){
    newPosition = BOTTOM_POS;
    writeIntIntoEEPROM(STEPPER_POS_ADDRESS,DEFAULT_POS);
    Serial.print(" >>>>>>>>>>>>>>> BOTTOM_POS [newPosition] written memory:");
    Serial.println(newPosition);
    if (newPosition != oldPosition && newPosition < oldPosition) {
      Serial.println("Driving stepper to Bottom Position");
      byte retn[4] = {_r, _e, _t, _n};
      disp.point(0);   
      disp.twistByte(retn, 25);

      digitalWrite(dirPin,LOW); 
      for(int x_init = oldPosition+newPosition; x_init >= 0; --x_init) {
        // Serial.print("x_init_down:");
        // Serial.println(x_init);
        digitalWrite(stepPin,HIGH); 
        delayMicroseconds(DELAY_MICROSEC); 
        digitalWrite(stepPin,LOW); 
        delayMicroseconds(DELAY_MICROSEC); 
      }
      oldPosition = newPosition; // Must be done after travel completion (after for-loop)
      Serial.println("\t::::::::::: Sensor Idling at BOTTOM_POS :::::::::::");

    }

  }


 } // end of void setup()


 void loop() {

    int distanceToObject = distanceSensor.measureDistanceCm();
    int height_cm = (SENSOR_HEIGHT - distanceToObject) - height_offset ; // 200-50=150
    // Serial.print("[PRESENCE ULTRASONIC] State: ");
    // Serial.print(personExist());
    // Serial.print("\tTo object:");
    // Serial.print(distanceToObject);
    // Serial.print("\tHeight:");
    // Serial.println(height_cm);

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
      disp.point(0);   
      disp.twistByte(here, 25);
    }
    else{
      disp.displayInt(distanceToObject);
    }
  

    // while (Serial.available() > 0) {
    //     Serial.println(":::Manual Input Controlled Linear Stepper:::");
    //     // read the incoming byte:
    //     height_cm = Serial.parseInt(); 

    while(personExist()){
        distanceToObject = distanceSensor.measureDistanceCm();
        height_cm = (SENSOR_HEIGHT - distanceToObject) - height_offset ; // 200-50=150

        if(height_cm < 140) height_cm = 140;
        if(height_cm > 200) height_cm = 200;

        Serial.print("[ULTRASONIC] Height received: ");
        Serial.print(height_cm, DEC);
        Serial.println(" cm");
        int mapped_height = map(height_cm, 140, 200, 0, 60);
        newPosition = map(mapped_height, 0, 60, 0, MAX_STEPS); 
      
        byte height[4];
        for (int i = 3 ; i >= 0 ; i--)
        {
          height[i] = height_cm % 10 ;
          height_cm /= 10 ;
        }
        disp.point(1);    
        disp.clear();
        disp.twistByte(height, 1);     
        disp.scroll(height, 100);     
        delay(DELAY_MICROSEC);

        Serial.print("!!!!!!!!!!!!!!!!!!!!!!!!!!! Moving to newPosition at: ");
        Serial.println(newPosition, DEC);
      
      if(DEFAULT_POS == BOTTOM_POS){ // sensor at BOTTOM_POS
        if (newPosition != oldPosition && newPosition > oldPosition) {
        if(personExist()){
          byte hold[4] = {_h, _o, _l, _d};
          disp.point(0);   
          disp.twistByte(hold, 25);
        }
        else{
          byte halt[4] = {_H, _a, _L, _t};
          disp.point(0);  
          disp.twistByte(halt, 25);
          Serial.println("############# Person missing! Aborting task....#############");
          delay(100);
          break;
        }
        digitalWrite(dirPin,HIGH); // go up away from stepper HOME
        for(int x = oldPosition; x <= newPosition; x++) {
          // Serial.print("[TO DESTINATION] x_up:"); Serial.println(x); // from 0 to MAX_STEPS
          digitalWrite(stepPin,HIGH); 
          delayMicroseconds(DELAY_MICROSEC); 
          digitalWrite(stepPin,LOW); 
          delayMicroseconds(DELAY_MICROSEC); 
        }
        oldPosition = newPosition; // IMPORTANT for returning back later! oldPosition will be referenced!

      }
    }
    else{ // sensor at TOP_POS
      if (newPosition != oldPosition && newPosition < oldPosition) {
        if(personExist()){
          byte hold[4] = {_h, _o, _l, _d};
          disp.point(0);   
          disp.twistByte(hold, 25);
        }
        else{
          byte halt[4] = {_H, _a, _L, _t};
          disp.point(0);  
          disp.twistByte(halt, 25);
          Serial.println("############# Person missing! Aborting task....#############");
          delay(100);
          break;
        }
      digitalWrite(dirPin,LOW); // go down towards stepper HOME
      for(int x = oldPosition; x >= newPosition; --x) {
        // Serial.print("[TO DESTINATION] x_down:"); Serial.println(x);  // from MAX_STEPS to 0
        digitalWrite(stepPin,HIGH); 
        delayMicroseconds(DELAY_MICROSEC); 
        digitalWrite(stepPin,LOW); 
        delayMicroseconds(DELAY_MICROSEC); 
      }
      oldPosition = newPosition; // IMPORTANT for returning back later! oldPosition will be referenced!

    }

    }
    Serial.println("\t::::::::::: Saving current pos immediately to memory :::::::::::");
    writeIntIntoEEPROM(STEPPER_POS_ADDRESS, newPosition); // this MUST be saved in case of power failure!
    Serial.println("\t::::::::::: \\\ Saving current pos DONE /// :::::::::::");

    oldPosition = readIntFromEEPROM(STEPPER_POS_ADDRESS);
    Serial.print("\t::::::::::: \\\ Loaded current pos/// :::::::::::");
    Serial.println(oldPosition);
    delay(100);

    disp.clear();
    byte scan[4] = {_5, _c, _a, _n};
    disp.point(0);   
    disp.twistByte(scan, 25);
    delay(250);

    Serial.println("\t:::::::::::Scanning temp (100ms delay):::::::::::");

    int head_temp = mlx.readObjectTempC();
    delay(100);
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
    disp.twistByte(tempc, 1);     
    disp.point(1);   
    disp.scroll(tempc, 100);     

    Serial.println("\t:::::::::::Scanning temp DONE:::::::::::");
    Serial.println("\t:::::::::::Going Home:::::::::::");
    newPosition = DEFAULT_POS; // Going Home after 250ms [RETURN BACK TO DEFAULT POSITION]

    if(DEFAULT_POS==BOTTOM_POS){
      if (newPosition != oldPosition && newPosition < oldPosition) {
        digitalWrite(dirPin,LOW);  // go down towards stepper HOME
        for(int x = oldPosition; x >= newPosition; --x) {
          // Serial.print("[RETURN] x_down:"); Serial.println(x); // from WHERE_EVER to BOTTOM_POS
          digitalWrite(stepPin,HIGH); 
          delayMicroseconds(DELAY_MICROSEC); 
          digitalWrite(stepPin,LOW); 
          delayMicroseconds(DELAY_MICROSEC); 
        }
        oldPosition = newPosition;

      }
    }
    else{
       digitalWrite(dirPin,HIGH); // go up away from stepper HOME
        for(int x = oldPosition; x <= newPosition; x++) {
          // Serial.print("[RETURN] x_up:"); Serial.println(x); // from WHERE_EVER to TOP_POS
          digitalWrite(stepPin,HIGH); 
          delayMicroseconds(DELAY_MICROSEC); 
          digitalWrite(stepPin,LOW); 
          delayMicroseconds(DELAY_MICROSEC); 
        }
        oldPosition = newPosition;

    }
    Serial.println("\t::::::::::: Saving current pos | IDLE position :::::::::::");
    writeIntIntoEEPROM(STEPPER_POS_ADDRESS, oldPosition); // this MUST be saved in case of power failure!
    Serial.println("\t::::::::::: Saving current pos | IDLE position DONE :::::::::::");

    byte done[4] = {_d, _o, _n, _e};
    disp.point(0);   
    disp.twistByte(done, 25);
    Serial.println(":::: IDLE ::::"); 
  }
    delay(1000); // master delay in this USER INPUT loop
    disp.clear();

  #else
    Serial.println(":::Automated Linear Stepper:::");
    digitalWrite(dirPin,HIGH); //Changes the rotations direction
    for(int x = 0; x < MAX_STEPS; x++) {
      digitalWrite(stepPin,HIGH);
      delayMicroseconds(DELAY_MICROSEC);
      digitalWrite(stepPin,LOW);
      delayMicroseconds(DELAY_MICROSEC);
    }
    delay(1000); // One second delay
    
    digitalWrite(dirPin,LOW); //Changes the rotations direction
    for(int x = 0; x < MAX_STEPS; x++) {
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
  disp.runningString(welcome_banner, sizeof(welcome_banner), 200);  // 200 это время в миллисекундах!
}

void running_TOP_POS() {
  byte welcome_banner[] = {_t, _O, _P, _empty, _P, _O, _5, _empty,        
                          };
  disp.runningString(welcome_banner, sizeof(welcome_banner), 200);  // 200 это время в миллисекундах!
}

void running_BOTTOM_POS() {
  byte welcome_banner[] = {_B, _O, _t,_t,_O, _empty, _P, _O, _5, _empty,        
                          };
  disp.runningString(welcome_banner, sizeof(welcome_banner), 200);  // 200 это время в миллисекундах!
}

 bool personExist(){

    int range_cm = ultrasonic.MeasureInCentimeters(); 
    if(range_cm >=0 && range_cm < 50) 
    {
      presence_detected = true;
      digitalWrite(PRESENCE_LED,1);
    }
    else{
      presence_detected = false;
      digitalWrite(PRESENCE_LED,0);

    }
    return presence_detected;              

 }