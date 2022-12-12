/*
Testing UiTM WiFi IoT 9/12/2022 Friday
Works with ESP32S2

*/
#define ESP32C3
/* Comment this out to disable prints and save space */
#define BLYNK_PRINT Serial
#define BLYNK_TEMPLATE_ID "TMPLTapmSTsx"

#ifndef ESP32C3
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
    String dateTime = get_timestamp();
    int RSSI_dBm =  WiFi.RSSI();
    Blynk.virtualWrite(V0,uptime_formatter::getUptime());
    Blynk.virtualWrite(V1, dateTime);
    Blynk.virtualWrite(V4, RSSI_dBm);

}


BLYNK_CONNECTED() {
  timeClient.setTimeOffset(28800);
  restart_ts = get_timestamp();
  Blynk.syncVirtual(V20,V21,V22);
  Blynk.virtualWrite(V2, WiFi.localIP().toString());
  Blynk.virtualWrite(V3, restart_ts); 
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

void setup()
{
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
