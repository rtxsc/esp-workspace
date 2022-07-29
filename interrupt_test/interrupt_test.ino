#include <util/atomic.h> // this library includes the ATOMIC_BLOCK macro.
#define INTERVAL_METHOD
 #define USE_TM1637 // comment this line if you dont have the display 
#define ENABLE_SERIAL_DEBUG

#ifdef USE_TM1637
#include <TM1637Display.h>
#endif

#define CLK                 8
#define DIO                 9
#define MINUTE_PER_HOUR     60
#define TICKS_PER_MINUTE    60
#define DIVISOR             1 // higher = faster update rate (need more magnets for better precision)
#define MAGNET_COUNT        20 // set your number of magnets here

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
volatile uint16_t ticks_per_sec = 0;
volatile uint8_t DIVISOR_VALUE = 1;

#define M1A 5
#define THROTTLE 0

uint16_t min_read = 0;
uint16_t max_read = 0;

bool minimum_obtained = false;
bool maximum_obtained = false;


long startMillis;
int elapse = 0;
double speed_kmh = 0.0;
double speed2_kmh = 0.0;
int rpm = 0;

float r = 0.056; // 5.6 cm or 0.056m (fan) /// 3.3cm or 0.033m (HDD 2.5") // 352mm for SunDing
float circf = 2 * pi * r; // 0.3518583772 meter
float dist = 0.0;

#ifdef USE_TM1637
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
  for(int x = 0; x < 50; x++) Serial.println();
  Serial.println("Speedometer Test using Arduino");
  startMillis = millis(); // start ticker

  #ifdef USE_TM1637
  int k;
  uint8_t data[] = { 0xff, 0xff, 0xff, 0xff };
  uint8_t blank[] = { 0x00, 0x00, 0x00, 0x00 };
  display.setBrightness(0x0f);
  display.setSegments(SEG_DONE);
  delay(500);

  for(k=0; k <= 4; k++) {
    display.showNumberDecEx((k+1)*1111, (0x80 >> k), true);
    delay(100);
  }
  #endif

  pinMode(M1A, OUTPUT); // PWM
  pinMode(THROTTLE, INPUT);
   
  Serial.println("Throttle Test & Encoder Speed Test");
  delay(1000);
  calibrate();

}

void loop() {
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
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

    ticks_per_sec = (tick * DIVISOR_VALUE);
    dist += ticks_per_sec * circf / 1000; // distance in km
    rpm = (ticks_per_sec * TICKS_PER_MINUTE ) / MAGNET_COUNT; // 1 tick / sec =  60 ticks per minute
    speed_kmh = (rpm * circf * MINUTE_PER_HOUR) / 1000; ; 

    /*

    single magnet precision
    1.26 kmh
    2.53 kmh
    3.80 kmh
    5.07 kmh
    6.33 kmh
    7.06 kmh
    8.87 kmh
    10.13 kmh
    11.40 kmh
    12.67 kmh

    4 magnets precision (more magnets = more precision)
    0.32
    0.63
    0.95
    1.27

    */

   #ifdef ENABLE_SERIAL_DEBUG
    Serial.print("Divisor:");
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

    int speed_int = (DIVISOR_VALUE*1000) + int(speed_kmh);

    #ifdef USE_TM1637
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
    rpm = (1000 / elapse * 60) / MAGNET_COUNT;
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


    } // ATOMIC END

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
  analogWrite(M1A, mapped_throttle);
  
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

  while(!maximum_obtained){
    Serial.print("Push throttle to max in:");
    for(int i=0; i<5; i++){
      Serial.print(5-i);
      Serial.print(" ");
      delay(100);
    }
    Serial.println();
    read_throttle = analogRead(THROTTLE);
    if(read_throttle < min_read + 50){
       Serial.println("Throttle not pushed to max! Please do so now!!!");
       delay(100);
    }
    else{
       max_read = read_throttle;
      Serial.println("Maximum throttle obtained");
      delay(100);
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