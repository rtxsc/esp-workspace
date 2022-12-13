/*
Testing UiTM WiFi IoT 9/12/2022 Friday
Works with ESP32S2

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

#include <WiFi.h>
#include <WiFiClient.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <BlynkSimpleEsp32.h>
#include "uptime_formatter.h"
#include "uptime.h"
#include "EEPROM.h"

#include <Adafruit_NeoPixel.h>
#define RGB         18 
#define NUMPIXELS   1 
#define DELAYVAL    100 

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
NTPClient           timeClient(ntpUDP, "pool.ntp.org", 3600, 60000);
BlynkTimer          timer;
Adafruit_NeoPixel   pixels(NUMPIXELS, RGB);

// Variables to save date and time
String formattedDate;
String dayStamp;
String timeStamp;
String restart_ts = "None";

#define restartCounterAddress   0x0F // 15 : 1 byte
#define ESP_RST_COUNTER_ADDR    0x10 // 16 : 1 byte
#define EEPROM_SIZE             32
#define FAST_DELAY              1000
byte restartCounter;      // value will be loaded from EEPROM
byte prev_restartCounter;

String get_timestamp(){
  timeClient.update();
  formattedDate = timeClient.getFormattedDate();
  // Extract date
  int splitT = formattedDate.indexOf("T");
  dayStamp = formattedDate.substring(0, splitT);
  String dateTime = timeClient.getFormattedTime() + " " + dayStamp;
  return dateTime;
}

void blynk_task(){
    if(millis() % 3 == 0)
      on_onboard_led();
    else
      off_onboard_led();    
    String dateTime = get_timestamp();
    int RSSI_dBm =  WiFi.RSSI();
    Blynk.virtualWrite(V0,uptime_formatter::getUptime());
    Blynk.virtualWrite(V1, dateTime);
    Blynk.virtualWrite(V4, RSSI_dBm);
}


BLYNK_CONNECTED() {
  timeClient.setTimeOffset(28800);
  restart_ts = get_timestamp();
  Blynk.syncVirtual(V20,V21,V22,V23);
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

 void clear_restartCounter(){
    Serial.println("CLEARIN RCounter");
    EEPROM.write(restartCounterAddress, 255);
    EEPROM.commit();
    vTaskDelay(1000 / portTICK_PERIOD_MS);
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


void setup()
{
  init_eeprom();
  restartCounter = EEPROM.read(restartCounterAddress);
  check_restart_count();
  pinMode(ONBOARD_LED, OUTPUT);
  #ifdef ESP32C3
    pinMode(R,OUTPUT);
    pinMode(G,OUTPUT);
    pinMode(B,OUTPUT);
  #endif
  off_onboard_led();
  // Debug console
  Serial.begin(115200);
  WiFi.mode(WIFI_STA); //Optional
  Blynk.begin(auth, ssid, pass);
  on_onboard_led();
  pixels.begin(); 
  loopRGB();
  loopRGB();
  rgbOff();
  timeClient.begin();
  timeClient.setTimeOffset(28800);
  timer.setInterval(1000L, blynk_task);
}


void loop()
{
  Blynk.run();
  timer.run();
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
