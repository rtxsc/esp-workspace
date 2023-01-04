/*
Testing UiTM WiFi IoT 9/12/2022 Friday
Works with ESP32S2
Edited 28 Dec 2022

*/
#define ESP32S2_2
/* Comment this out to disable prints and save space */
#define BLYNK_PRINT Serial
#define BLYNK_TEMPLATE_ID "TMPLTapmSTsx"

#ifndef ESP32S2_2
  #define BLYNK_DEVICE_NAME "AASAS ONE"
  #define BLYNK_AUTH_TOKEN "mi-P1ww34-Z1hCIHdsZY_zBiChxDmFW3"
#else
  #define BLYNK_DEVICE_NAME "AASAS TWO"
  #define BLYNK_AUTH_TOKEN "wl51qgH6H_DDeSC-7FvB39wQJ3d7ic8V"
#endif
#include <Wire.h>
#include "rgb_lcd.h"

#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>
#include "uptime_formatter.h"
#include "uptime.h"
#include "EEPROM.h"

#include <Adafruit_NeoPixel.h>
#include <Adafruit_INA219.h>

#include <NTPClient.h>
#include <WiFiUdp.h>

#define RGB         18 
#define NUMPIXELS   1 
#define DELAYVAL    100 
#define IN1         19
#define IN2         20
#define IN3         21

#ifdef ESP32C3
  #define ONBOARD_LED 18
  #define R 3
  #define G 4
  #define B 5
#else
  #define ONBOARD_LED 2
#endif
//#define BLYNK_AUTH_TOKEN "3G4XbLzWHurLKwzeAeKQZH7QttvcM9gR"   // AASAS ONE SERVER
//#define BLYNK_AUTH_TOKEN "WMPQFiXeWmh7xHHUsngi8oyIHO4bG47D"   // AASAS TWO SERVER

// You should get Auth Token in the Blynk App.
// Go to the Project Settings (nut icon).
char auth[] = BLYNK_AUTH_TOKEN;

// Your WiFi credentials.
// Set password to "" for open networks.
char ssid[] = "UiTM WiFi IoT";
char pass[] = ""; // leave this empty as this is an open network

//char ssid[] = "Maxis Postpaid 128 5G";
//char pass[] = "respironics"; // leave this empty as this is an open network
WiFiUDP             ntpUDP;
// NTPClient           timeClient(ntpUDP, "pool.ntp.org", 3600, 60000);
NTPClient           timeClient(ntpUDP);
BlynkTimer          timer;
Adafruit_NeoPixel   pixels(NUMPIXELS, RGB);
Adafruit_INA219     ina219_A;
rgb_lcd             lcd;

// Variables to save date and time
String formattedDate;
String dayStamp;
String timeStamp;
String restart_ts = "None";

float shuntvoltage;
float busvoltage;
float current_mA;

bool GROVE_LCD_AVAILABLE = false;
bool INA219_AVAILABLE = false;
#define I2C_SDA                 41
#define I2C_SCL                 40
#define restartCounterAddress   0x0F // 15 : 1 byte
#define ESP_RST_COUNTER_ADDR    0x10 // 16 : 1 byte
#define EEPROM_SIZE             32
#define FAST_DELAY              1000
byte restartCounter;      // value will be loaded from EEPROM
byte prev_restartCounter;


 byte degree_symbol[8] = {
    0b00110,
    0b01001,
    0b01001,
    0b00110,
    0b00000,
    0b00000,
    0b00000,
    0b00000,
  };
    byte right_arrow[8] = {
    0b10000,
    0b11000,
    0b11100,
    0b11110,
    0b11111,
    0b11110,
    0b11100,
    0b11000,
  };
    byte wave_right[8] = {
    0b11000,
    0b11100,
    0b01110,
    0b00111,
    0b00111,
    0b01110,
    0b11100,
    0b11000,
  };

  byte wave_left[8] = {
    0b00011,
    0b00111,
    0b01110,
    0b11100,
    0b11100,
    0b01110,
    0b00111,
    0b00011,
  };

BLYNK_CONNECTED();
BLYNK_WRITE(V20);
BLYNK_WRITE(V21);
BLYNK_WRITE(V22);
BLYNK_WRITE(V23);
BLYNK_WRITE(V24);
BLYNK_WRITE(V25);
BLYNK_WRITE(V26);

void BLYNK_TASK();
void init_eeprom();
void i2c_scan();
String get_timestamp();
void init_eeprom();
void off_onboard_led();
void on_onboard_led();
void loopRGB();
void redOn();
void greenOn();
void blueOn();
void rgbOff();
void check_restart_count();
void clear_restartCounter();
void try_wifi_connect();

void onRelay1();
void onRelay2();
void onRelay3();
void offRelay1();
void offRelay2();
void offRelay3();

void setup()
{
  Wire.begin(I2C_SDA, I2C_SCL);
  Serial.println("\nScanning ESP32s2 i2c port...");
  i2c_scan();
  if(INA219_AVAILABLE){
    initINA219();
  }
  if(GROVE_LCD_AVAILABLE){
    lcd.begin(16, 2);
    lcd.createChar(1, wave_right); // create block character
    lcd.createChar(2, wave_left); // create block character
    lcd.createChar(3, right_arrow); // create block character
    lcd.createChar(4, degree_symbol); // create block character
    lcd.setCursor(0,0);
    lcd.print("Hello AASAS TWO!");
    lcd.setCursor(0,1);
    lcd.print("Connecting WiFi");
    delay(1000);
  }  
  

  init_eeprom();
  restartCounter = EEPROM.read(restartCounterAddress);
  check_restart_count();
  pinMode(ONBOARD_LED, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);

  #ifdef ESP32C3
    pinMode(R,OUTPUT);
    pinMode(G,OUTPUT);
    pinMode(B,OUTPUT);
  #endif
  off_onboard_led();
  // Debug console
  Serial.begin(115200);
  WiFi.mode(WIFI_STA); //Optional
  if(GROVE_LCD_AVAILABLE){
    try_wifi_connect(); // pre-connection using WiFi.begin()
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Connecting Blynk");
    lcd.setCursor(0,1);
    lcd.write(1); // add arrow before SSID
    lcd.print(ssid);
  }
  // 30 Dec 2022 Friday
  // /Users/zidz/Documents/Arduino/libraries/Blynk/src/Adapters/BlynkArduinoClient.h (to fix connecting to blynk.cloud:80 infinite loop)
  // /Users/zidz/Documents/Arduino/libraries/Blynk/src/Blynk/BlynkProtocol.h (to edit logo)
  // /Users/zidz/Documents/Arduino/libraries/Blynk/src/BlynkSimpleEsp32.h (to edit Blynk.begin method)
  Blynk.begin(auth, ssid, pass); // Blynk begin ignoring WiFi cuz already connected =)
  if(GROVE_LCD_AVAILABLE){
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Blynk Connected!");
    lcd.setCursor(0,1);
    lcd.print("-AASAS TWO INIT-");
  }
  on_onboard_led();
  pixels.begin(); 
  loopRGB();
  loopRGB();
  rgbOff();
  timeClient.begin();
  // timeClient.setTimeOffset(28800);
  timer.setInterval(1000L, BLYNK_TASK);
}


void loop()
{
  Blynk.run();
  timer.run();
}

void try_wifi_connect(){
    int init_index_right = 9; // init 8 + 1 offset
    int init_index_left = 6; // init 7 - 1 offset
    int column_index_right = init_index_right; 
    int column_index_left = init_index_left;

    int wl_count = 0;
    int connect_elapse = 2; // second unit
    WiFi.begin(ssid, pass); // normal connect method
    while (WiFi.status() != WL_CONNECTED) {
          lcd.setCursor(column_index_right, 1);
          lcd.write(1);
          column_index_right++;

          lcd.setCursor(init_index_left+1, 1);
          lcd.print("T");
          lcd.setCursor(init_index_left+2, 1);
          lcd.print(String(wl_count/10));

          lcd.setCursor(column_index_left, 1);
          lcd.write(2);
          column_index_left--;

          if(column_index_right>16 && column_index_left<0) {
            #ifdef USE_RGB_LED
              #ifndef AASAS_TPLINK
              rgbOff();
              #endif
            #endif
            column_index_right  = init_index_right; 
            column_index_left   = init_index_left;
            lcd.clear();
            if(millis()/1000 % 2 == 0){
                lcd.setCursor(0, 0); // row 1, column 0
                lcd.print("Connecting WiFi");  
                off_onboard_led();

            }
            else{
                lcd.setCursor(0, 0); // row 1, column 0
                lcd.print(String(ssid));
                on_onboard_led();
            }
          }

      wl_count++;
      vTaskDelay(100 / portTICK_PERIOD_MS);
      
      if(wl_count > connect_elapse*10 && (WiFi.status() != WL_CONNECTED)){
          wl_count = 0;
          lcd.clear();
          lcd.setCursor(0, 0); // row 1, column 0
          lcd.print("Not Connected!");
          lcd.setCursor(0, 1); // row 1, column 0
          lcd.print("Restarting ESP32");
          ESP.restart();
      }
    }
    if(GROVE_LCD_AVAILABLE){
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("-WiFi Connected-");
      lcd.setCursor(0,1);
      lcd.print("--UiTM WiFi IoT-");
      vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}


String get_timestamp(){
  timeClient.update();
  // https://github.com/taranais/NTPClient.git
  formattedDate = timeClient.getFormattedDate(); // getFormattedDate
  // Serial.println(formattedDate);
  // Extract date
  int splitT = formattedDate.indexOf("T");
  dayStamp = formattedDate.substring(4, splitT);
  String dateTime = timeClient.getFormattedTime() + " " + dayStamp;
  return dateTime;
}


void off_onboard_led(){
   #ifdef ESP32C3
      digitalWrite(ONBOARD_LED, LOW); // LOW    = OFF LED on ESP32 Dev and C3
   #else
      digitalWrite(ONBOARD_LED, HIGH); // HIGH  = OFF LED on ESP32S2
   #endif  
}

void on_onboard_led(){
  #ifdef ESP32C3
    digitalWrite(ONBOARD_LED, HIGH); // HIGH  = ON LED on ESP32 Dev and C3
  #else
    digitalWrite(ONBOARD_LED, LOW); // LOW    = ON LED on ESP32S2
  #endif
}

void loopRGB(){
 
  pixels.setPixelColor(0, pixels.Color(127, 0, 0));
  pixels.show();   
  delay(DELAYVAL); 
  pixels.setPixelColor(0, pixels.Color(0, 127, 0));
  pixels.show();   
  delay(DELAYVAL); 
  pixels.setPixelColor(0, pixels.Color(0, 0, 127));
  pixels.show();   
  delay(DELAYVAL); 
  pixels.setPixelColor(0, pixels.Color(0, 0, 0));
  pixels.show();   
  delay(DELAYVAL); 
   pixels.setPixelColor(0, pixels.Color(0, 0, 0));
  pixels.show();   
  delay(DELAYVAL); 
}

void onRelay1(){
  digitalWrite(IN1,1);
}
void offRelay1(){
  digitalWrite(IN1,0);
}
void onRelay2(){
  digitalWrite(IN2,1);
}
void offRelay2(){
  digitalWrite(IN2,0);
}
void onRelay3(){
  digitalWrite(IN3,1);
}
void offRelay3(){
  digitalWrite(IN3,0);
}

void redOn(){
  #ifdef ESP32C3
  digitalWrite(R,1);
  digitalWrite(G,0);
  digitalWrite(B,0);
  #else
  pixels.setPixelColor(0, pixels.Color(127, 0, 0));
  pixels.show(); 
  #endif  
}
void greenOn(){
  #ifdef ESP32C3
  digitalWrite(R,0);
  digitalWrite(G,1);
  digitalWrite(B,0);
  #else
  pixels.setPixelColor(0, pixels.Color(0, 127, 0));
  pixels.show(); 
  #endif    
}
void blueOn(){
  #ifdef ESP32C3
  digitalWrite(R,0);
  digitalWrite(G,0);
  digitalWrite(B,1);
  #else
  pixels.setPixelColor(0, pixels.Color(0, 0, 127));
  pixels.show(); 
  #endif    
}

void rgbOff(){
  #ifdef ESP32C3
  digitalWrite(R,0);
  digitalWrite(G,0);
  digitalWrite(B,0);
  #else
  pixels.setPixelColor(0, pixels.Color(0, 0, 0));
  pixels.show(); 
  #endif   
  }

 void clear_restartCounter(){
    if(GROVE_LCD_AVAILABLE){
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("-Reset ESP32S2!-");
      lcd.setCursor(0,1);
      lcd.print("-Restarting Now-");
    }
    Serial.println("@@@@@@@@@@@@@@@@@@ CLEARING Reset Counter @@@@@@@@@@@@@@@@@@ ");
    EEPROM.write(restartCounterAddress, 255);
    EEPROM.commit();
    vTaskDelay(10 / portTICK_PERIOD_MS);
    ESP.restart();
 
}

void check_restart_count(){
  if(restartCounter==255 ){
    // lcd.print("IT'S NEW NODE");
    // lcd.setCursor(0, 1); // row 1, column 0
    // lcd.print("Reset Rc Counter"); // Reboot 000 times
    // vTaskDelay(FAST_DELAY / portTICK_PERIOD_MS);
    // lcd.clear();
    restartCounter = 0;
    EEPROM.write(restartCounterAddress, restartCounter);
    EEPROM.commit();
    vTaskDelay(3.3 / portTICK_PERIOD_MS); // EEPROM needs 3.3ms to write

  }
  else{
      Serial.println("just a normal reboot or restart");
      // lcd.setCursor(0, 0); 
      // lcd.print("REBOOTED NODE");
      // lcd.setCursor(0, 1); // row 1, column 0
      // lcd.print("Restarted "+String(restartCounter) + " tms");
      // vTaskDelay(FAST_DELAY / portTICK_PERIOD_MS);
      // lcd.clear();
      
      prev_restartCounter = restartCounter; // for the comparison later on
      restartCounter += 1;  // increment rst count by 1 for each reboot
      Serial.printf("the current restart count is %d\n", restartCounter);
      // lcd.setCursor(0, 0); 
      // lcd.print("--WELCOME BACK--"); 
      // lcd.setCursor(0, 1); // row 1, column 0
      // lcd.print("Current RST "+String(restartCounter) + " times"); // load current rst count
      // vTaskDelay(FAST_DELAY / portTICK_PERIOD_MS);
      // lcd.clear();

      EEPROM.write(restartCounterAddress, restartCounter);
      EEPROM.commit();
      vTaskDelay(3.3 / portTICK_PERIOD_MS); // EEPROM needs 3.3ms to write          
  }
}

void init_eeprom(){

  if (!EEPROM.begin(EEPROM_SIZE))
  {
    // lcd.clear();
    // lcd.setCursor(0, 0); 
    // lcd.print("-CRITICAL ERROR-");
    // lcd.setCursor(0, 1); 
    // lcd.print("-EEPROM  FAILED-");
    // vTaskDelay(1000000 / portTICK_PERIOD_MS);
    Serial.println("failed to initialise EEPROM"); delay(1000000);

  }

  Serial.println(" bytes read from Flash:");
  for (int i = 0; i < EEPROM_SIZE; i++)
  {
    Serial.print("R "); Serial.print(i);   Serial.print(":");
    Serial.print(byte(EEPROM.read(i))); Serial.println();
  }
  Serial.println();
}

void initINA219(){
  if (! ina219_A.begin()) {
    Serial.println("Failed to find INA219_A chip");
    lcd.setCursor(0, 0); // row 1, column 0
    lcd.print("INA219_A FAILED!");
    lcd.setCursor(0, 1); // row 1, column 0
    lcd.print("CHECK I2C WIRE");
    while (1) { delay(10); }
  }
  ina219_A.setCalibration_32V_2A();
}

void getINA219(){
  shuntvoltage = ina219_A.getShuntVoltage_mV(); // measured across Vin+ & Vin-
  busvoltage = ina219_A.getBusVoltage_V(); // measured at Vin+ only
  current_mA = ina219_A.getCurrent_mA();
}

void i2c_scan() {
  byte error, address;
  int nDevices;
  Serial.println("Scanning...");
  nDevices = 0;
  for(address = 1; address < 127; address++ ) {
    digitalWrite(2, HIGH);   // turn the LED on (HIGH is the voltage level)
    digitalWrite(18, HIGH); 
    delay(15);                       // wait for a second
    digitalWrite(2, LOW);    // turn the LED off by making the voltage LOW
    digitalWrite(18, LOW); 
    delay(15);                       // wait for a second
    Wire.beginTransmission(address);
    error = Wire.endTransmission();
    if (error == 0) {
      Serial.print("I2C device found at address 0x");
      if (address<16) {
        Serial.print("0");
      }
      Serial.println(address,HEX);
      if(address == 0x3E) GROVE_LCD_AVAILABLE = true;
      if(address == 0x40) INA219_AVAILABLE    = true;
      nDevices++;
    }
    else if (error==4) {
      Serial.print("Unknow error at address 0x");
      if (address<16) {
        Serial.print("0");
      }
      Serial.println(address,HEX);
    }    
  }
  if (nDevices == 0) {
    Serial.println("No I2C devices found\n");
    digitalWrite(2, HIGH);   // turn the LED on (HIGH is the voltage level)
    digitalWrite(18, HIGH); 
    delay(100);                       // wait for a second
    digitalWrite(2, LOW);    // turn the LED off by making the voltage LOW
    digitalWrite(18, LOW); 
    delay(500);                       // wait for a second
  }
  else {
    Serial.println("done\n");
  }
  delay(2000);          
}

void display_uptime_top_row(){
    uptime_formatter::getUptime();
    lcd.setCursor(0, 0); // row 0, column 0
    lcd.print("U");
    if(int(uptime::getDays())<10)
    {lcd.print("00");
    lcd.print(uptime::getDays());}
    else if(int(uptime::getDays())<100)
    {lcd.print("0");
    lcd.print(uptime::getDays());}
    else
    lcd.print(uptime::getDays());

    lcd.print("d ");
    lcd.setCursor(6, 0);
    if(int(uptime::getHours())<10){
      lcd.print("0");
      lcd.print(uptime::getHours());
    }
    else
      lcd.print(uptime::getHours());
      lcd.print("h ");
      lcd.setCursor(10, 0);
      if(int(uptime::getMinutes())<10){
        lcd.print("0");
        lcd.print(uptime::getMinutes());
      }
    else
      lcd.print(uptime::getMinutes());
    lcd.print("m ");
    lcd.setCursor(14, 0);
    lcd.print(uptime::getSeconds());
    lcd.print("s");
  }

void BLYNK_TASK(){
    if(millis() % 3 == 0)
      on_onboard_led();
    else
      off_onboard_led();    
    String dateTime = get_timestamp();
    int RSSI_dBm =  WiFi.RSSI();
    Blynk.virtualWrite(V0,uptime_formatter::getUptime());
    Blynk.virtualWrite(V1, dateTime);
    Blynk.virtualWrite(V4, RSSI_dBm);
    getINA219();
    Blynk.virtualWrite(V6, busvoltage);
    Blynk.virtualWrite(V7, current_mA); 
    display_uptime_top_row();
    lcd.setCursor(0,1);
    lcd.print(dateTime);
}


BLYNK_CONNECTED() {
  timeClient.setTimeOffset(28800);
  restart_ts = get_timestamp();
  Blynk.syncVirtual(V20,V21,V22,V23,V24,V25,V26);
  Blynk.virtualWrite(V2, WiFi.localIP().toString());
  Blynk.virtualWrite(V3, restart_ts); 
  Blynk.virtualWrite(V5, restartCounter);
}



BLYNK_WRITE(V20){
  int pinValue = param.asInt();
  if(pinValue) redOn();
  else rgbOff();
}

BLYNK_WRITE(V21){
  int pinValue = param.asInt();
  if(pinValue) greenOn();
  else rgbOff();
}

BLYNK_WRITE(V22){
  int pinValue = param.asInt();
  if(pinValue) blueOn();
  else rgbOff();
}

BLYNK_WRITE(V23){
  int pinValue = param.asInt();
  if(pinValue) clear_restartCounter();
}

BLYNK_WRITE(V24){
  int val = param.asInt();
  if(val) onRelay1();
  else    offRelay1();
}

BLYNK_WRITE(V25){
  int val = param.asInt();
  if(val) onRelay2();
  else    offRelay2();
}
BLYNK_WRITE(V26){
  int val = param.asInt();
  if(val) onRelay3();
  else    offRelay3();
}

