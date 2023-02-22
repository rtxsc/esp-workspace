// latest update 28.06.2022 00:43
#include "GyverTM1637.h" // for BIG VERSION
#include <HCSR04.h>
#include <Wire.h>
// #include <Adafruit_GFX.h>
// #include <Adafruit_SSD1306.h>
#include <Adafruit_MLX90614.h>
#include "Ultrasonic.h"
#include <EEPROM.h>
#include "cnc_shield_uno.h"
#include <Encoder.h>

#include <Adafruit_NeoPixel.h>
#define LED_PIN     A1
#define RING_PIN    A2
#define LED_COUNT   40
#define RING_COUNT  8
Adafruit_NeoPixel   strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel   strip_ring(RING_COUNT, RING_PIN, NEO_GRB + NEO_KHZ800);


#define DELAY_MICROSEC  500
#define DELAY_100MS     100


#define DUMMY_BMI
// #define AUTO_CONTROL
// #define AUTO_DUMMY //  used for product features showcase
// #define ENCODER_CONTROL
// #define AUTO_LOOP
// #define CNC_SHIELD // comment this out if not using CNC Shield
// #define CHANGE_EMISSIVITY
// #define TEST_HEIGHT_SENSOR
// #define DEBUG_MODE

/*
Pin usage CPS-HPV
D2 - A4988 X.STEP
D3 - Gyver CLK
D4 - Gyver DIO
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


HW-434 Setup

HANPOSE 17HS4401S Vref Config
Phase Current (I): 1.7A
Consider 10% lower than rated current
I = 1.7 - (1.7x0.1) = 1.53 A
Vref = I x 8 x Rsense = 1.53A x 8 x 0.1R = 1.224V

*/

#define SENSOR_HEIGHT       200
#define MAX_STEPS           3090      
#define STEPPER_POS_ADDRESS 0
#define height_offset       0 // 12 - standard HCSR04 || 0 - Grove HCSR04

#define SCREEN_WIDTH        128 
#define SCREEN_HEIGHT       64 
#define OLED_RESET          -1 // Reset pin # (or -1 if sharing Arduino reset pin)

#ifdef CNC_SHIELD
  Encoder myEnc(Z_POS_NEG, SpnEn);
  #define CLK Y_STEP  // GYVER D3
  #define DIO Z_STEP  // GYVER D4
  #define PRESENCE_LED Z_POS_NEG
  #define MODE_SELECT SpnDir // D13
  #define BUZZER        8
  // Stepper Motor X
  const uint8_t stepPin = X_STEP; //X.STEP D2
  const uint8_t dirPin = X_DIR; // X.DIR D5
#else
  // D2 available if not in ENCODER_CONTROL mode
  #define CLK           3  // GYVER
  #define DIO           4  // GYVER
  // D5 Presence Trig
  // D6 Presence Echo
  // D7 Height Grove Sig
  #define BUZZER        8
  #define stepPin       9  // gray
  #define dirPin        10 // blue
  #define enablePin     11
  // D11 Available if not in ENCODER_CONTROL mode

  #define PRESENCE_LED  12 // not utilized yet
  #define MODE_SELECT   13 // DO NOT DISTURB THIS PIN

#ifdef ENCODER_CONTROL
  Encoder myEnc(2, 11); // using the spare D2 and D11 for now
#endif
  int x; // global indexing for Encoder position 24.06.2022
#endif


byte height_sense[4];
bool presence_detected = false;
uint16_t DEFAULT_POS;       // will cause unavoidable low memory issue 24.06.2022
uint16_t oldPosition  = 0;  // change to uint16_t 24.06.2022
uint16_t newPosition;       // change to uint16_t 24.06.2022

GyverTM1637               disp(CLK, DIO);
#ifdef CNC_SHIELD
UltraSonicDistanceSensor  presence_sensor(Y_DIR, Z_DIR);  // normal HCSR04 - Presence
Ultrasonic                height_sensor(X_POS_NEG);       // Grove HCSR04 - Height
#else
UltraSonicDistanceSensor  presence_sensor(5, 6);          // normal HCSR04 - Presence (TRIG,ECHO)
Ultrasonic                height_sensor(7);               // Grove HCSR04 - Height
#endif

Adafruit_MLX90614         mlx = Adafruit_MLX90614();

// Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
// int eeprom_val; //  disabled for memory reservation


#ifdef CHANGE_EMISSIVITY
  double new_emissivity = 0.8; // default at E=1
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
  Serial.begin(9600);
  pinMode(MODE_SELECT, INPUT_PULLUP);
  pinMode(BUZZER, OUTPUT);
  pinMode(stepPin,OUTPUT); 
  pinMode(dirPin,OUTPUT);
  pinMode(enablePin,OUTPUT);
  pinMode(PRESENCE_LED,OUTPUT); // LED at D10 as presence status

  strip.begin();           // INITIALIZE NeoPixel strip object (REQUIRED)
  strip.show();            // Turn OFF all pixels ASAP
  strip.setBrightness(50);

  strip_ring.begin();           // INITIALIZE NeoPixel strip object (REQUIRED)
  strip_ring.show();            // Turn OFF all pixels ASAP
  strip_ring.setBrightness(50);

  rainbow(20); 
  rainbow_ring(20);

  // for(int addr = 0; addr < 5; addr++){
  //   eeprom_val = EEPROM.read(addr);
  //   Serial.print("Memory ");
  //   Serial.print(addr);
  //   Serial.print("\t");
	//   Serial.print(eeprom_val);
	//   Serial.println();
  //   delay(100);
  // }
  mlx.begin(); 
  disp.clear();
  disp.brightness(7);  // яркость, 0 - 7 (минимум - максимум)
  byte test[4] = {_t, _e, _S, _t};
  disp.point(0);   
  disp.twistByte(test, 25);
  #ifdef CHANGE_EMISSIVITY
    // read current emissivity
    Serial.print("Current emissivity = "); Serial.println(mlx.readEmissivity());
    // set new emissivity
    Serial.print("Setting emissivity = "); Serial.println(new_emissivity);
    mlx.writeEmissivity(new_emissivity); // this does the 0x0000 erase write
    // read back
    Serial.print("New emissivity = "); Serial.println(mlx.readEmissivity());  // if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3D for 128x64
  #endif
  delay(1000);
  disp.displayInt(mlx.readEmissivity()*1000);
  delay(1000);

  #ifdef AUTO_CONTROL
  beep_once();
  delay(500);
  beep_twice();
  delay(500);
  beep_thrice();
  delay(500);
  // siren();
  // delay(500);
 
  oldPosition = readIntFromEEPROM(STEPPER_POS_ADDRESS);
  if(oldPosition==255){
      Serial.println(":::Stepper Default Position Not Saved in Memory:::");
      delay(1000);
  }
  Serial.print("DELAY_MICROSECOND:");
  Serial.println(DELAY_MICROSEC);
  Serial.println(":::Auto HCSR04 Input Controlled Linear Stepper:::");
  welcomeScroll();
  randomSeed(analogRead(A0)); // disabled 24.06.2022

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
  #endif

  disable_stepper();

 } // end of void setup()


 void dummy_bmi(int height_cm , float weight){

   /*
      If your BMI is less than 18.5, it falls within the underweight range.
      If your BMI is 18.5 to 24.9, it falls within the Healthy Weight range.
      If your BMI is 25.0 to 29.9, it falls within the overweight range.
      If your BMI is 30.0 or higher, it falls within the obese range.
   
   */

    // float weight = 82.05; // frans weight as reference
    // int height_cm = 151 ; // 200-50=150 frans height as reference

    float h_m = height_cm / 100.0;
    int w_x100 =  weight * 100;
    float bmi = (weight / (h_m * h_m));
    int bmi_x100 = bmi * 100;

    if(height_cm < 140) height_cm = 140;
    if(height_cm > 200) height_cm = 200;

    byte height[4];
    for (int i = 3 ; i >= 0 ; i--)
    {
      height[i] = height_cm % 10 ;
      height_cm /= 10 ;
    }
    byte tall[4] = {_t, _a, _l, _l};
    disp.point(0);   
    disp.scrollByte(tall, 50);
    delay(DELAY_MICROSEC);

    disp.point(1);    
    disp.clear();
    disp.twistByte(height, 1);     
    disp.scroll(height, 100);     
    delay(1000);

    byte weight_arr[4];
    for (int i = 3 ; i >= 0 ; i--)
    {
      weight_arr[i] = w_x100 % 10 ;
      w_x100 /= 10 ;
    }
    byte load[4] = {_l, _o, _a, _d};
    disp.point(0);   
    disp.scrollByte(load, 50);
    delay(DELAY_MICROSEC);
    disp.point(1);    
    disp.clear();
    disp.twistByte(weight_arr, 1);     
    disp.scroll(weight_arr, 100);     
    delay(1000);


    byte bmi_arr[4];
    for (int i = 3 ; i >= 0 ; i--)
    {
      bmi_arr[i] = bmi_x100 % 10 ;
      bmi_x100 /= 10 ;
    }


    byte bnni[4] = {_b, _n, _n, _i};
    disp.point(0);   
    disp.scrollByte(bnni, 50);
    delay(DELAY_MICROSEC);

    disp.point(1);    
    disp.clear();
    disp.twistByte(bmi_arr, 1);     
    disp.scroll(bmi_arr, 100);     
    delay(1000);

    clearStrip();
    if(bmi < 18.5){
      // underweight
      colorWipeToBMI(strip.Color(0,0,255), 20, bmi); // blue
    }else if(bmi >= 18.5 && bmi <= 24.9){
      // healthy weight
      colorWipeToBMI(strip.Color(0,255,0), 20, bmi); // green
    }
    else if(bmi >= 25 && bmi <= 29.9){
      // overweight
      colorWipeToBMI(strip.Color(100,0,0), 20, bmi); // light red
    }
    else{
      // obese
      colorWipeToBMI(strip.Color(255,0,0), 20, bmi); // bright red
    }
    delay(1000);
 } // end of bmi func

void clearStrip(){
  colorWipe(strip.Color(0,0,0), 0);
}

 void loop() {

  #ifdef DUMMY_BMI
  float weight_fran = 82.05;
  float weight_yazid = 60;
  float weight_squiter = 65;
  float weight_evie = 50;
  rainbow(10);
  dummy_bmi(170, weight_evie);
  rainbow(10);
  dummy_bmi(160, weight_yazid);
  rainbow(10);
  dummy_bmi(158, weight_squiter);
  rainbow(10);
  dummy_bmi(151, weight_fran);

  #endif

  #ifdef TEST_HEIGHT_SENSOR

  int distanceToObject = height_sensor.MeasureInCentimeters();
  int height_cm = (SENSOR_HEIGHT - distanceToObject) - height_offset ; // 200-50=150

  if(height_cm < 140) height_cm = 140;
  if(height_cm > 200) height_cm = 200;

  Serial.print("[ULTRASONIC] Height received: ");
  Serial.print(height_cm, DEC);
  Serial.println(" cm");

  int LOWEST_STEP = 10;
  int mapped_height = map(height_cm, 140, 200, 0, 60);
  newPosition = map(mapped_height, 0, 60, LOWEST_STEP, MAX_STEPS); 

  byte height[4];
  for (int i = 3 ; i >= 0 ; i--)
  {
    height[i] = height_cm % 10 ;
    height_cm /= 10 ;
  }
  disp.clear();
  disp.point(1);    
  // disp.twistByte(height, 1);     
  disp.scroll(height, 100);  
  delay(DELAY_MICROSEC);
  #endif

  #ifdef ENCODER_CONTROL
  /* polished logic 24.06.2022 Friday 23:41PM */
  int newPosition = (myEnc.read() / 2) * 200; // 200 x 1.8deg = 360deg per Encoder step
  if(newPosition < 0) newPosition = 0;
  if(newPosition > MAX_STEPS) newPosition = MAX_STEPS;

  if(newPosition != oldPosition){
    Serial.println(":::Encoder Controlled Linear Stepper:::");
    if(newPosition > oldPosition){
      Serial.print("[+NEW POS UP++] :"); Serial.println(newPosition); // from 0 to MAX_STEPS
    }
    else{
      Serial.print("[-NEW POS DOWN] :"); Serial.println(newPosition); // from MAX_STEPS to 0
    }
  }

  if (newPosition != oldPosition && newPosition > oldPosition) {
    digitalWrite(dirPin,HIGH); // Enables the motor to move in a particular direction
    // Makes 200 pulses for making one full cycle rotation
    for(x = oldPosition; x <= newPosition; x++) {
      //  Serial.print("[TO DESTINATION] x_up:"); Serial.println(x); // from 0 to MAX_STEPS
       digitalWrite(stepPin,HIGH); 
       delayMicroseconds(DELAY_MICROSEC); 
       digitalWrite(stepPin,LOW); 
       delayMicroseconds(DELAY_MICROSEC); 
    }
    oldPosition = newPosition;
  }

  if (newPosition != oldPosition && newPosition < oldPosition) {
    digitalWrite(dirPin,LOW); // Enables the motor to move in a particular direction
    // Makes 200 pulses for making one full cycle rotation
    for(x = oldPosition; x >= newPosition; --x) {
      // Serial.print("[TO DESTINATION] x_down:"); Serial.println(x);  // from MAX_STEPS to 0
      digitalWrite(stepPin,HIGH); 
      delayMicroseconds(DELAY_MICROSEC); 
      digitalWrite(stepPin,LOW); 
      delayMicroseconds(DELAY_MICROSEC); 
    }
    oldPosition = newPosition;
  }

  delay(100);

  #elif defined AUTO_CONTROL

    // use Grove HC-SR04
    // MeasureInCentimeters - Grove
    // measureDistanceCm - Standard
    disable_stepper();

    #ifdef AUTO_DUMMY
    int distanceToObject = 140;
    #else
    int distanceToObject = height_sensor.MeasureInCentimeters();
    #endif
    int height_cm = (SENSOR_HEIGHT - distanceToObject) - height_offset ; // 200-50=150
    if(height_cm < 0) height_cm = 0;
    // Serial.print("[PRESENCE ULTRASONIC] State: ");
    // Serial.print(personExist());
    // Serial.print("\tTo object:");
    // Serial.print(distanceToObject);
    // Serial.print("\tHeight:");
    // Serial.println(height_cm);

    if(millis() % 5 == 0){

      byte get_[4] = {_empty,_G, _e, _t};
      disp.point(0);   
      disp.twistByte(get_, 25);
      delay(50);
      byte here[4] = {_h, _e, _r, _e};
      disp.point(0);   
      disp.twistByte(here, 25);
    }
    else{
      disp.displayInt(height_cm);
    }

    rainbow(10); 
    // disp.displayInt(height_cm);

    // while (Serial.available() > 0) {
    //     Serial.println(":::Manual Input Controlled Linear Stepper:::");
    //     // read the incoming byte:
    //     height_cm = Serial.parseInt(); 

    while(personExist()){

        // use Grove HC-SR04
        // MeasureInCentimeters - Grove
        // measureDistanceCm - Standard

        byte read[4] = {_r, _e, _a, _d};
        disp.point(0);   
        disp.scrollByte(read, 25);
        delay(DELAY_100MS);
    
        byte retry = 0;
        do{
          retry += 1;
          distanceToObject = height_sensor.MeasureInCentimeters();
          #ifdef AUTO_DUMMY
          height_cm = 140;
          #else
          height_cm = (SENSOR_HEIGHT - distanceToObject) - height_offset ; // 200-50=150
          #endif

          if(height_cm < 140){
            byte low[4] = {_empty, _empty, _L, _O};
            disp.point(0);   
            disp.scrollByte(low, 25);
            delay(DELAY_100MS);
          }else{
            byte good[4] = {_G, _o, _o, _d};
            disp.point(0);   
            disp.scrollByte(good, 25);
            delay(DELAY_100MS);

          }

          if(!personExist()) break;

          disp.displayInt(height_cm);
          delay(DELAY_100MS);
        } while(height_cm < 140 || height_cm > 200 && retry<5);

        if(height_cm < 140) height_cm = 140;
        if(height_cm > 200) height_cm = 200;

        #ifdef DEBUG_MODE
        Serial.print("[ULTRASONIC] Height received: ");
        Serial.print(height_cm, DEC);
        Serial.println(" cm");
        #endif

        int LOWEST_STEP = 10;
        int mapped_height = map(height_cm, 140, 200, 0, 60);
        newPosition = map(mapped_height, 0, 60, LOWEST_STEP, MAX_STEPS); 
      
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

        #ifdef DEBUG_MODE
        Serial.print("!!!!!!!!!!!!!!!!!!!!!!!!!!! Moving to newPosition at: ");
        Serial.println(newPosition, DEC);
        #endif
      
      if(DEFAULT_POS == BOTTOM_POS){ // sensor at BOTTOM_POS
        if (newPosition != oldPosition && newPosition > oldPosition) {
        if(personExist()){
          byte hold[4] = {_h, _o, _l, _d};
          disp.point(0);   
          disp.twistByte(hold, 25);
        }
        else{
          personMissing();
          byte halt[4] = {_H, _a, _L, _t};
          disp.point(0);  
          disp.twistByte(halt, 25);
          Serial.println("############# Person missing! Aborting task....#############");
          delay(DELAY_100MS);
          break;
        }
        enable_stepper();
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
          personMissing();
          byte halt[4] = {_H, _a, _L, _t};
          disp.point(0);  
          disp.twistByte(halt, 25);
          Serial.println("############# Person missing! Aborting task....#############");
          delay(DELAY_100MS);
          break;
        }
      enable_stepper();
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
    // Serial.println("\t::::::::::: Saving current pos immediately to memory :::::::::::");
    writeIntIntoEEPROM(STEPPER_POS_ADDRESS, newPosition); // this MUST be saved in case of power failure!
    // Serial.println("\t::::::::::: \\\ Saving current pos DONE /// :::::::::::");

    oldPosition = readIntFromEEPROM(STEPPER_POS_ADDRESS);
    #ifdef DEBUG_MODE
    Serial.print("\t::::::::::: \\\ Loaded current pos/// :::::::::::");
    Serial.println(oldPosition);
    #endif
    delay(DELAY_100MS);

    disp.clear();
    byte scan[4] = {_5, _c, _a, _n};
    disp.point(0);   
    disp.twistByte(scan, 25);
    delay(DELAY_100MS);

    // Serial.println("\t:::::::::::Scanning temp (100ms delay):::::::::::");

    float head_temp = mlx.readObjectTempC(); // change to float 24.06.2022
    delay(DELAY_100MS);

    #ifdef DEBUG_MODE
    Serial.print("forehead temp:");
    Serial.println(head_temp);
    #endif

    byte tempc[4];
    int tempc_int = head_temp*100; // random(3500,3900);
    // int tempc_int = random(3600,3700);

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
    // normal 35.4 °C and 37.4 °C.
    if(head_temp >= 0 && head_temp < 35){
      colorWipe(strip.Color(  0,   0, 255), 10); // Blue
      beep_twice(); // abnormally low
    }
    else if(head_temp >= 35 && head_temp < 37.5){
      colorWipe(strip.Color(  0, 255,   0), 10); // Green
      beep_once(); // normal
    }  
    else if(head_temp >= 37.5 && head_temp <= 39){
      colorWipe(strip.Color(128,   0,   0), 10); // Light Red
      beep_thrice(); // above normal
    } 
    else{
      colorWipe(strip.Color(255,   0,   0), 10); // Red
      siren(); // abnormally high
    }

    /*
    colorWipe(strip.Color(255,   0,   0), 50); // Red
    colorWipe(strip.Color(  0, 255,   0), 50); // Green
    colorWipe(strip.Color(  0,   0, 255), 50); // Blue
    */
    
    #ifdef DEBUG_MODE
    Serial.println("\t:::::::::::Scanning temp DONE:::::::::::");
    Serial.println("\t:::::::::::Going Home:::::::::::");
    #endif
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
    // Serial.println("\t::::::::::: Saving current pos | IDLE position :::::::::::");
    writeIntIntoEEPROM(STEPPER_POS_ADDRESS, oldPosition); // this MUST be saved in case of power failure!
    // Serial.println("\t::::::::::: Saving current pos | IDLE position DONE :::::::::::");

    byte done[4] = {_d, _o, _n, _e};
    disp.point(0);   
    disp.twistByte(done, 25);
    Serial.println(":::: IDLE ::::"); 
  }
    disable_stepper();
    delay(1000); // master delay in this USER INPUT loop
    disp.clear();

  #elif defined AUTO_LOOP
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
 } // end of void loop

void welcomeScroll() {
  byte welcome_banner[] = {_H, _E, _L, _L, _O, _empty, _C, _P, _S, _empty,       
                          };
  disp.runningString(welcome_banner, sizeof(welcome_banner), 200);  // 200 это время в миллисекундах!
}

void running_TOP_POS() {
  byte welcome_banner[] = {_t, _o, _empty,_t, _O, _P, _empty, _P, _O, _5, _empty,        
                          };
  disp.runningString(welcome_banner, sizeof(welcome_banner), 200);  // 200 это время в миллисекундах!
}

void running_BOTTOM_POS() {
  byte welcome_banner[] = {_t, _o, _empty,_B, _O, _t,_t,_O, _empty, _P, _O, _5, _empty,        
                          };
  disp.runningString(welcome_banner, sizeof(welcome_banner), 200);  // 200 это время в миллисекундах!
}

 bool personExist(){
    // use standard HC-SR04
    // MeasureInCentimeters - Grove
    // measureDistanceCm - Standard

    int range_cm = presence_sensor.measureDistanceCm(); 
    if(range_cm >=0 && range_cm < 50) 
    {
      presence_detected = true;
      digitalWrite(PRESENCE_LED,1);
      personDetected();
    }
    else{
      presence_detected = false;
      digitalWrite(PRESENCE_LED,0);
      personNotDetected();
    }
    #ifdef AUTO_DUMMY
    return 1;              
    #else
    return presence_detected;              
    #endif
 }

void beep_once(){
  tone(8, 1500, 150);
}

void beep_twice(){
  tone(8, 1500, 50);
  delay(100);
  tone(8, 1500, 50);
  delay(100);
}

void beep_thrice(){
  tone(8, 1500, 50);
  delay(100);
  tone(8, 1500, 50);
  delay(100);
  tone(8, 1500, 50);
  delay(100);
}

void siren(){
  int min_tone = 1000;
  int max_tone = 1500;
  for(int i=min_tone ; i <= max_tone ; i++){
      tone(8, i, 20);
      delay(1);        // delay in between reads for stability
  }
  for(int i=max_tone ; i > min_tone ; i--){
      tone(8, i, 20);
      delay(1);        // delay in between reads for stability
  }
}

void enable_stepper(){
  digitalWrite(enablePin, LOW);
}

void disable_stepper(){
  digitalWrite(enablePin, HIGH);
}

// Rainbow cycle along whole strip. Pass delay time (in ms) between frames.
void rainbow(int wait) {
  // Hue of first pixel runs 5 complete loops through the color wheel.
  // Color wheel has a range of 65536 but it's OK if we roll over, so
  // just count from 0 to 5*65536. Adding 256 to firstPixelHue each time
  // means we'll make 5*65536/256 = 1280 passes through this loop:
  for(long firstPixelHue = 0; firstPixelHue < 1*65536; firstPixelHue += 2048) {
    // strip.rainbow() can take a single argument (first pixel hue) or
    // optionally a few extras: number of rainbow repetitions (default 1),
    // saturation and value (brightness) (both 0-255, similar to the
    // ColorHSV() function, default 255), and a true/false flag for whether
    // to apply gamma correction to provide 'truer' colors (default true).
    strip.rainbow(firstPixelHue);
    // Above line is equivalent to:
    // strip.rainbow(firstPixelHue, 1, 255, 255, true);
    strip.show(); // Update strip with new contents
    delay(wait);  // Pause for a moment
  }
}

void rainbow_ring(int wait) {
  for(long firstPixelHue = 0; firstPixelHue < 1*65536; firstPixelHue += 2048) {
    strip_ring.rainbow(firstPixelHue);
    strip_ring.show(); // Update strip with new contents
    delay(wait);  // Pause for a moment
  }
}

void colorWipeToBMI(uint32_t color, int wait, float bmi) {

   /*
      If your BMI is less than 18.5, it falls within the underweight range.
      If your BMI is 18.5 to 24.9, it falls within the Healthy Weight range.
      If your BMI is 25.0 to 29.9, it falls within the overweight range.
      If your BMI is 30.0 or higher, it falls within the obese range.
   */

  int maxWipe = map(bmi, 18.5, 30.0, 8, strip.numPixels());

  for(int i=0; i<maxWipe; i++) { // For each pixel in strip...
    strip.setPixelColor(i, color);         //  Set pixel's color (in RAM)
    strip.show();                          //  Update strip to match
    delay(wait);                           //  Pause for a moment
  }
}

void colorWipe(uint32_t color, int wait) {
  for(int i=0; i<strip.numPixels(); i++) { // For each pixel in strip...
    strip.setPixelColor(i, color);         //  Set pixel's color (in RAM)
    strip.show();                          //  Update strip to match
    delay(wait);                           //  Pause for a moment
  }
}

void colorWipeRing(uint32_t color, int wait) {
  for(int i=0; i<strip_ring.numPixels(); i++) { // For each pixel in strip...
    strip_ring.setPixelColor(i, color);         //  Set pixel's color (in RAM)
    strip_ring.show();                          //  Update strip to match
    delay(wait);                           //  Pause for a moment
  }
}

/*
    colorWipeRing(strip_ring.Color(  0,   0, 255), 50); // Blue - Person Missing
    delay(10);
    colorWipeRing(strip_ring.Color(  0,   255, 0), 50); // Green - Person Detected
    delay(10);
    colorWipeRing(strip_ring.Color(  255,  0, 0), 50); // Red - Person Not Detected
    delay(10);
*/

void personDetected(){
    colorWipeRing(strip_ring.Color(  0,   255, 0), 50); // Green - Person Detected
}

void personNotDetected(){
    colorWipeRing(strip_ring.Color(  255,  0, 0), 50); // Red - Person Not Detected
}

void personMissing(){
    colorWipeRing(strip_ring.Color(  0,   0, 255), 50); // Blue - Person Missing

}