
/* Fill-in your Template ID (only if using Blynk.Cloud) */
#define BLYNK_TEMPLATE_ID "TMPLLcUZS8pw"
#define BLYNK_DEVICE_NAME "AFS"
#define BLYNK_AUTH_TOKEN "3G4XbLzWHurLKwzeAeKQZH7QttvcM9gR"
/* Comment this out to disable prints and save space */
#define BLYNK_PRINT Serial
//#define USE_RTC

#include <WiFi.h>
#include <ESP32Ping.h>
#include <WiFiClient.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

#include <BlynkSimpleEsp32.h>
#include <Wire.h>
#include "rgb_lcd.h"
#include "DS1307.h"
#include "uptime_formatter.h"
#include "uptime.h"
#define relay_in1   17
#define relay_in2   25
#define relay_in3   27
#define relay_in4   16
#define state0_vpin 30
#define state1_vpin 31
#define state2_vpin 32
#define state3_vpin 33

// You should get Auth Token in the Blynk App.
// Go to the Project Settings (nut icon).
char auth[] = BLYNK_AUTH_TOKEN;

// Your WiFi credentials.
// Set password to "" for open networks.
char ssid[] = "NPRDC CELCOM M1";
char pass[] = "nprdc1234";
const char* remote_host = "blynk.cloud";
WiFiClient  client;
DS1307      _clock;
BlynkTimer  timer;
rgb_lcd     lcd;
WiFiUDP     ntpUDP;
NTPClient   timeClient(ntpUDP, "pool.ntp.org", 3600, 60000);

// Variables to save date and time
String formattedDate;
String dayStamp;
String timeStamp;

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

BLYNK_CONNECTED() {
  Blynk.syncVirtual(V0, V1, V2, V3);
}

BLYNK_WRITE(V0)
{
  int pinValue = param.asInt(); // assigning incoming value from pin V1 to a variable
  digitalWrite(relay_in1,pinValue);
  Blynk.virtualWrite(state0_vpin, digitalRead(relay_in1));
}
BLYNK_WRITE(V1)
{
  int pinValue = param.asInt(); // assigning incoming value from pin V1 to a variable
  digitalWrite(relay_in2,pinValue);
  Blynk.virtualWrite(state1_vpin, digitalRead(relay_in2));
}
BLYNK_WRITE(V2)
{
  int pinValue = param.asInt(); // assigning incoming value from pin V1 to a variable
  digitalWrite(relay_in3,pinValue);
  Blynk.virtualWrite(state2_vpin, digitalRead(relay_in3));
}
BLYNK_WRITE(V3)
{
  int pinValue = param.asInt(); // assigning incoming value from pin V1 to a variable
  digitalWrite(relay_in4,pinValue);
  Blynk.virtualWrite(state3_vpin, digitalRead(relay_in4));
}

void myTimerEvent()
{
  Blynk.virtualWrite(V10, millis() / 1000);
}

void setup()
{
  Serial.begin(115200);
  lcd.begin(16, 2);
  lcd.createChar(1, wave_right); // create block character
  lcd.createChar(2, wave_left); // create block character
  lcd.createChar(3, right_arrow); // create block character
  lcd.createChar(4, degree_symbol); // create block character

  pinMode(relay_in1,OUTPUT);
  pinMode(relay_in2,OUTPUT);
  pinMode(relay_in3,OUTPUT);
  pinMode(relay_in4,OUTPUT);

  #ifdef USE_RTC
    _clock.begin();
    _clock.fillByYMD(2021,12,24);
    _clock.fillByHMS(12,30,30);
    _clock.fillDayOfWeek(FRI);
    _clock.setTime();
  #endif

  lcd.setCursor(0,0);
  lcd.print("Welcome to AFS");
  lcd.setCursor(0,1);
  lcd.print("Connecting WiFi");
  delay(1000);
  WiFi.begin(ssid, pass);
  long connectingTime = millis();

  int init_index_right = 9; // init 8 + 1 offset
  int init_index_left = 6; // init 7 - 1 offset
  int column_index_right = init_index_right; 
  int column_index_left = init_index_left;

  int wl_count = 0;
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
          #ifdef USE_BLUE_LED
            digitalWrite(BLUE_LED, HIGH); 
          #elif defined USE_RGB_LED 
            #ifndef AASAS_TPLINK
            setRGBtoWhite();
            #endif
            onOK_LED();
            onSELECT_LED();
          #endif

          }
          else{
              lcd.setCursor(0, 0); // row 1, column 0
              lcd.print(String(ssid));
          #ifdef USE_BLUE_LED
            digitalWrite(BLUE_LED, LOW); 
          #elif defined USE_RGB_LED
            #ifndef AASAS_TPLINK
            rgbOff();
            #endif
            offOK_LED();
            offSELECT_LED();
          #endif  
          }
      
        }

    wl_count++;
    vTaskDelay(100 / portTICK_PERIOD_MS);
    
    if(wl_count>100 && (WiFi.status() != WL_CONNECTED)){
        wl_count = 0;
        lcd.clear();
        lcd.setCursor(0, 0); // row 1, column 0
        lcd.print("Failed 2 Connect");
        lcd.setCursor(0, 1); // row 1, column 0
        lcd.print("Restarting ESP32");
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        ESP.restart();
    }

  }
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Connecting Blynk");
  lcd.setCursor(0,1);
  lcd.print("blynk.cloud:80");

  Blynk.begin(auth, ssid, pass);
  Serial.print("[BLYNK WiFi Handler] WiFi connected with IP ");  
  Serial.println(WiFi.localIP());

  lcd.setCursor(0,0);
  lcd.print("!WiFi Connected!");
  lcd.setCursor(0,1);
  lcd.print("-Using NTP Time-");
  delay(1000);

  timeClient.begin();
  timeClient.setTimeOffset(28800);

  Blynk.virtualWrite(V12,WiFi.localIP().toString());
  //timer.setInterval(1000L, myTimerEvent);
  #ifdef USE_RTC
    timer.setInterval(1000L, printTimeRTC);
  #else
    timer.setInterval(1000L, printTimeNTP);
  #endif

//  timer.setInterval(15000L, Get_Ping);
}

void loop()
{
  
  Blynk.run();
  timer.run();
}

void Get_Ping() 
{
  Ping.ping(remote_host,1);
  int avg_time_ms = Ping.averageTime();
  Blynk.virtualWrite(V21,avg_time_ms);
}

void printTimeNTP(){
  lcd.clear();
  timeClient.update();
  formattedDate = timeClient.getFormattedDate();
  // Extract date
  int splitT = formattedDate.indexOf("T");
  dayStamp = formattedDate.substring(0, splitT);
  String dateTime = timeClient.getFormattedTime() + " " + dayStamp;
  display_uptime_top_row();
  lcd.setCursor(0,1);
  lcd.print(dateTime);
  Blynk.virtualWrite(V10,uptime_formatter::getUptime());
  Blynk.virtualWrite(V11, dateTime);

}

void printTimeRTC()
{
  lcd.clear();
  _clock.getTime();
  String dateTime = String(_clock.hour) + ":" + String(_clock.minute) + ":" + String(_clock.second) + " " + 
                  String(_clock.month) + "/" + String(_clock.dayOfMonth) + "/" + String(_clock.year+2000) ;
  
  String dateTimeLcd = String(_clock.hour) + ":" + String(_clock.minute) + ":" + String(_clock.second) + " " + 
                  String(_clock.month) + "/" + String(_clock.dayOfMonth);
  lcd.setCursor(0, 1);
  lcd.print(dateTimeLcd);
  Blynk.virtualWrite(V11, dateTime);
  Blynk.virtualWrite(V10,uptime_formatter::getUptime());
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
