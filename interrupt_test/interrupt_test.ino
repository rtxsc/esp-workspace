#include <util/atomic.h> // this library includes the ATOMIC_BLOCK macro.
#define INTERVAL_METHOD
 #define USE_TM1637 // comment this line if you dont have the display 
// #define ENABLE_SERIAL_DEBUG

#ifdef USE_TM1637
#include <TM1637Display.h>
#endif

#define CLK                 6
#define DIO                 7
#define MINUTE_PER_HOUR     60
#define TICKS_PER_MINUTE    60
#define DIVISOR             1 // higher = faster update rate (need more magnets for better precision)

#define pi 3.141592653589793238

#ifdef USE_TM1637
TM1637Display display(CLK, DIO);
#endif


const int ELAPSE_ONE_SECOND = 1000;
const byte ledPin = 13;
const byte button = 2;
const byte interruptPin = 3;
volatile byte state = LOW;
volatile uint16_t tick = 0;
volatile uint32_t ticks_per_sec = 0;
volatile uint32_t rpm = 0;
uint32_t avg_rpm = 0;
uint32_t prev_rpm = 0 ;
byte DIVISOR_VALUE = 1;
uint8_t HOLES_COUNT   = 20 ; // set your number of magnets here

#define M1A 5
#define THROTTLE 0

uint16_t min_read = 0;
uint16_t max_read = 0;

bool minimum_obtained = false;
bool maximum_obtained = false;


const uint8_t read_size = 8;
int array_storage[read_size];   // the array_storage from the analog input
int readIndex = 0;              // the index of the current reading
int total = 0;                  // the running total
int average = 0;                // the average


long startMillis;
double speed_kmh = 0.0;
double speed2_kmh = 0.0;
bool idle_checked = false;
long checkIdle = 0;
// default to 0278
// r =  motorcycle 17 inch rim (55.78 cm diameter / 27.89 cm radius / 0.28m radius)
double r = 0.278; // 5.6 cm or 0.056m (fan) /// 3.3cm or 0.033m (HDD 2.5") // 352mm for SunDing
float circf = 0;
float dist = 0.0;

#ifdef USE_TM1637

const uint8_t SEG_RAD[] = {
	SEG_E | SEG_G ,                                  // R
	SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_G,   // a
	SEG_B | SEG_C | SEG_D | SEG_E | SEG_G,           // d
	SEG_B | SEG_C                                    // I
	};

  const uint8_t SEG_YES[] = {
    SEG_B | SEG_C | SEG_D | SEG_F | SEG_G,           // Y
	  SEG_A | SEG_B | SEG_D | SEG_E | SEG_F | SEG_G,   // e
	  SEG_A | SEG_C | SEG_D | SEG_F | SEG_G,            // S
    SEG_D
	};

  const uint8_t SEG_NO[] = {
    SEG_D,
    SEG_D,
	  SEG_C | SEG_E | SEG_G,                           // n
	  SEG_C | SEG_D | SEG_E | SEG_G                    // o
	};

const uint8_t SEG_INIT[] = {
	SEG_B | SEG_C ,                                  // I
	SEG_C | SEG_E | SEG_G,                           // n
	SEG_B | SEG_C ,                                  // I
	SEG_D | SEG_E | SEG_F | SEG_G                    // t
	};

const uint8_t SEG_PUSH[] = {
	SEG_A | SEG_B | SEG_E | SEG_F | SEG_G,          // P
	SEG_B | SEG_C | SEG_D | SEG_E | SEG_F,          // U
	SEG_A | SEG_C | SEG_D | SEG_F | SEG_G ,         // S
	SEG_B | SEG_C | SEG_E | SEG_F | SEG_G           // H
	};

const uint8_t SEG_H020[] = {
	SEG_B | SEG_C | SEG_E | SEG_F | SEG_G ,         // H
	SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F,  // 0
	SEG_A | SEG_B | SEG_D | SEG_E | SEG_G ,         // 2
  SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F   // 0
	};

const uint8_t SEG_H100[] = {
	SEG_B | SEG_C | SEG_E | SEG_F | SEG_G ,         // H
	SEG_B | SEG_C,                                  // 1
  SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F,  // 0
  SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F   // 0
	};

const uint8_t SEG_DONE[] = {
	SEG_B | SEG_C | SEG_D | SEG_E | SEG_G,           // d
	SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F,   // O
	SEG_C | SEG_E | SEG_G,                           // n
	SEG_A | SEG_D | SEG_E | SEG_F | SEG_G            // E
	};
#endif

void setup() {
  pinMode(ledPin, OUTPUT);
  pinMode(button, INPUT_PULLUP);
  pinMode(interruptPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(button), get_button_press, FALLING);
  attachInterrupt(digitalPinToInterrupt(interruptPin), get_tick, FALLING);
  Serial.begin(115200);
  reset_array();  
  for(int x = 0; x < 50; x++) Serial.println();
  Serial.println("Speedometer Test using Arduino");
  startMillis = millis(); // start ticker

  #ifdef USE_TM1637
  int k;
  uint8_t data[] = { 0xff, 0xff, 0xff, 0xff };
  uint8_t blank[] = { 0x00, 0x00, 0x00, 0x00 };
  display.setBrightness(0x0f);
  display.setSegments(SEG_INIT);
  delay(1000);

  for(k=0; k <= 4; k++) {
    display.showNumberDecEx((k+1)*1111, (0x80 >> k), true);
    delay(100);
  }
  #endif

  pinMode(M1A, OUTPUT); // PWM
  pinMode(THROTTLE, INPUT);
   
  Serial.println("Throttle Test & Encoder Speed Test");
  delay(1000);

  int startTime = millis();
  beep_once();
  while(millis()- startTime < 2000){
      if(DIVISOR_VALUE % 2 == 0){
        display.setSegments(SEG_H100);
        HOLES_COUNT = 100 ;
      }
      else{
        display.setSegments(SEG_H020);
        HOLES_COUNT = 20 ;
      }
  }
  display.setSegments(SEG_RAD);
  delay(1000);
  bool set_radius = false;

  for(int i=0; i<5; i++){
      Serial.print(5-i);
      Serial.print(" ");
      // display.showNumberDecEx(5-i, (0x80 >> 0), true);
      if(DIVISOR_VALUE % 2 == 0){
        Serial.println("Set Radius");
        display.setSegments(SEG_YES);
        set_radius = true;
      }
      else{
        Serial.println("Do Not Set Radius");
        display.setSegments(SEG_NO);
        set_radius = false;
      }
      delay(250);
    }
  
  if(set_radius){
    r = config_radius();
    Serial.print("confirmed new radius:");
    Serial.print(r,3);
  }
  else{
    Serial.print("default radius:");
    Serial.print(r,3);
  }
  circf = 2 * pi * r; 

  Serial.print("\tconfirmed circumference:");
  Serial.print(circf*1000);
  Serial.println(" mm");
  DIVISOR_VALUE = 1; // reset back for DIVISOR_VALUE actual purpose


  calibrate();
}

void loop() {
  
  // code with interrupts blocked (consecutive atomic operations will not get interrupted)
  // digitalWrite(ledPin, state);
  // ELAPSE_ONE_SECOND/DIVISOR is the scaler for update rate
  // default to 1000 / 10 = 100 millisecond = 0.1 second

  #ifdef INTERVAL_METHOD
  drive_motor();
  if(DIVISOR_VALUE > 10) DIVISOR_VALUE = 1;

  // ticks_per_sec = (tick * DIVISOR_VALUE);
  // Serial.print("Divisor:");
  // Serial.print(DIVISOR_VALUE);
  // Serial.print("\tTick:");
  // Serial.print(tick);
  // Serial.print("\tTick/sec:");
  // Serial.println(ticks_per_sec);
  // delay(1000);

  if(millis()-startMillis >= ELAPSE_ONE_SECOND/DIVISOR_VALUE){

    #ifdef ENABLE_SERIAL_DEBUG
    Serial.print("HOLES:");
    Serial.print(HOLES_COUNT);
    Serial.print("  Divisor:");
    Serial.print(DIVISOR_VALUE);
    Serial.print("\tTick:");
    Serial.print(tick);
    Serial.print("\tTick/sec:");
    Serial.print(ticks_per_sec);
    Serial.print("\tDistance:");
    Serial.print(dist);
    Serial.print(" km\tRPM:");
    Serial.print(rpm);
    Serial.print("  Speed:");
    Serial.print(speed_kmh);
    Serial.println(" km/h");
    #endif

  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
  /* --- ATOMIC NOT UTILIZED --- */
    ticks_per_sec = (tick * DIVISOR_VALUE);
    dist += (ticks_per_sec * circf / 1000) / HOLES_COUNT; // distance in km
  } // ATOMIC END

    rpm = (ticks_per_sec * TICKS_PER_MINUTE ) / HOLES_COUNT; // 1 tick / sec =  60 ticks per minute
    avg_rpm = smoothing_rpm(rpm);
    speed_kmh = (avg_rpm * circf * MINUTE_PER_HOUR) / 1000;

    #ifdef USE_TM1637
    int speed_int = (DIVISOR_VALUE*1000) + int(speed_kmh);
    display.showNumberDecEx(speed_int, (0x80 >> 0), true);
    #endif
    tick = 0; // reset tick to zero
    startMillis = millis(); // reset millis()
  }

  #else
  // ifdef ELAPSE_METHOD
  if(tick > 0){
    #ifdef USE_TM1637
    display.showNumberDecEx(speed2_kmh*10, (0x80 >> 2), false);
    #endif
    elapse = millis() - startMillis;
    rpm = (1000 / elapse * 60) / HOLES_COUNT;
    startMillis = millis();

    if(rpm > 5400) rpm = 5400;
    speed2_kmh = (rpm * circf * MINUTE_PER_HOUR) / 1000; // circ = 1.2568 when r=0.2
    // speed2_kmh = floor(speed2_kmh);
    
    #ifdef ENABLE_SERIAL_DEBUG

    Serial.print("Tick:");
    Serial.print(tick);
    tick = 0;

    Serial.print("\telapse:");
    Serial.print(elapse);
    Serial.print("\trpm:");
    Serial.print(rpm);
    Serial.print("\tSpeed:");
    Serial.print(speed2_kmh);
    Serial.println(" km/h");
    #endif

  }
  if(tick == 0 && millis()-startMillis > 2000){
      rpm = 0;
      speed2_kmh = (rpm * circf * MINUTE_PER_HOUR) / 1000;
      #ifdef ENABLE_SERIAL_DEBUG
      Serial.print("elapse:");
      Serial.print(elapse);
      Serial.print("\trpm:");
      Serial.print(rpm);
      Serial.print("\tSpeed:");
      Serial.print(speed2_kmh);
      Serial.println(" km/h");
      #endif
      delay(1000);
  }


  #endif



 } // END OF LOOP

void get_tick() {
  tick++;
}

void get_button_press(){
  DIVISOR_VALUE++;
}

void drive_motor(){
  int read_throttle = analogRead(THROTTLE);
  if(read_throttle < min_read)   min_read = read_throttle;
  if(read_throttle > max_read)   max_read = read_throttle;
  int mapped_throttle = map(read_throttle, min_read, max_read, 0, 255);

  // int mapped_throttle = map(read_throttle, min_read, max_read, 0, 255);
  // analogWrite(M1A, mapped_throttle);

  if(!idle_checked && (millis() - checkIdle > 1000) && rpm < 100 && read_throttle < min_read + 100){
    checkIdle = millis();
    digitalWrite(M1A, 0);
    idle_checked = true;
  }
  else{
    if((read_throttle < min_read + 50) && tick ==0){
      idle_checked = false;
    }else{
      analogWrite(M1A, mapped_throttle);
    }
  }

  
  // Serial.print("Raw:");
  // Serial.print(read_throttle);
  // Serial.print("\tMin:");
  // Serial.print(min_read);
  // Serial.print("\tMax:");
  // Serial.print(max_read);
  // Serial.print("\tThrottle:");
  // Serial.println(mapped_throttle);

}


void calibrate(){
   // put your main code here, to run repeatedly:
  int read_throttle = analogRead(THROTTLE);
  if(read_throttle < min_read)   min_read = read_throttle;
  if(read_throttle > max_read)   max_read = read_throttle;

  if(!minimum_obtained){
    min_read = read_throttle;
    minimum_obtained = true;
    Serial.println("Minimum throttle obtained");
    delay(100);
  }

  display.setSegments(SEG_PUSH);
  delay(1000);
  while(!maximum_obtained){
    Serial.print("Push throttle to max in:");
    for(int i=0; i<5; i++){
      Serial.print(5-i);
      Serial.print(" ");
      display.showNumberDecEx(5-i, (0x80 >> 0), true);
      delay(250);
    }
    Serial.println();
    read_throttle = analogRead(THROTTLE);
    if(read_throttle < min_read + 50){
       Serial.println("Throttle not pushed to max! Please do so now!!!");
       delay(100);
       beep_thrice();
    }
    else{
      max_read = read_throttle;
      Serial.println("Maximum throttle obtained");
      delay(100);
      beep_twice();

      display.setSegments(SEG_DONE);

      while(read_throttle > min_read + 50){
        read_throttle = analogRead(THROTTLE); // to check if the throttle is held max
        Serial.println("Release the throttle to begin...");
        delay(100);
      }
      maximum_obtained = true;
    }
   delay(100);

  }
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
void reset_array(){
  for (int thisReading = 0; thisReading < read_size; thisReading++) {
    array_storage[thisReading] = 0;
  }
}

int smoothing_rpm(int rpm){
    total = total - array_storage[readIndex];
    array_storage[readIndex] = rpm;
    total = total + array_storage[readIndex];
    readIndex = readIndex + 1;
    if (readIndex >= read_size) {
      readIndex = 0;
    }
    avg_rpm = total / read_size; // average rpm
    return avg_rpm;
}

double config_radius(){
    uint8_t data[] = { 0x00, 0x00, 0x00, 0x00 };
    uint8_t blank[] = { 0x00, 0x00, 0x00, 0x00 };

    byte digit0 = 0, digit1 = 0, digit2 = 0, digit3 = 0;
    bool confirm_digit0 = false;
    bool confirm_digit1 = false;
    bool confirm_digit2 = false;
    bool confirm_digit3 = false;

    while(1){

        while(!confirm_digit0){
          int read_throttle = analogRead(THROTTLE);
          if(read_throttle > 500) confirm_digit0 = true;
          else confirm_digit0 = false;

          if(!digitalRead(button)) digit0++;
          if(digit0 > 9) digit0 = 0;
          data[0] = display.encodeDigit(digit0);
          display.setSegments(data);
          delay(100);
          display.setSegments(blank);
          delay(100);

        }
        beep_twice();
        data[0] = display.encodeDigit(digit0);
        display.setSegments(data);
        Serial.println("digit 0 confirmed!");
        delay(1000);

        while(!confirm_digit1){
          int read_throttle = analogRead(THROTTLE);
          if(read_throttle > 500) confirm_digit1 = true;
          else confirm_digit1 = false;
          if(!digitalRead(button)) digit1++;
          if(digit1 > 9) digit1 = 0;
          data[1] = display.encodeDigit(digit1);
          display.setSegments(data);
          delay(100);
          display.setSegments(blank);
          delay(100);
   
        }
        beep_twice();
        data[0] = display.encodeDigit(digit0);
        data[1] = display.encodeDigit(digit1);
        display.setSegments(data);        
        Serial.println("digit 1 confirmed!");
      
        delay(1000);
        while(!confirm_digit2){
          int read_throttle = analogRead(THROTTLE);
          if(read_throttle > 500) confirm_digit2 = true;
          else confirm_digit2 = false;

          if(!digitalRead(button)) digit2++;
          if(digit2 > 9) digit2 = 0;
          data[2] = display.encodeDigit(digit2);
          display.setSegments(data);
          delay(100);
          display.setSegments(blank);
          delay(100);

        }
        beep_twice();
        data[0] = display.encodeDigit(digit0);
        data[1] = display.encodeDigit(digit1);
        data[2] = display.encodeDigit(digit2);
        display.setSegments(data);
        Serial.println("digit 2 confirmed!");
        
        delay(1000);
        while(!confirm_digit3){
          int read_throttle = analogRead(THROTTLE);
          if(read_throttle > 500) confirm_digit3 = true;
          else confirm_digit3 = false;

          if(!digitalRead(button)) digit3++;
          if(digit3 > 9) digit3 = 0;
          data[3] = display.encodeDigit(digit3);
          display.setSegments(data);
          delay(100);
          display.setSegments(blank);
          delay(100);

        }
        beep_thrice();
        data[0] = display.encodeDigit(digit0);
        data[1] = display.encodeDigit(digit1);
        data[2] = display.encodeDigit(digit2);
        data[3] = display.encodeDigit(digit3);
        Serial.println("digit 3 confirmed!");
        display.setSegments(data);
        delay(100);

        double rad = (digit0*1000 + digit1*100 + digit2*10 + digit3)/1000.0;
        return rad;
    }
}
