#include <Wire.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include "DS1307.h"

WiFiUDP             ntpUDP;
NTPClient           timeClient(ntpUDP);
DS1307              _clock;

#define I2C_SDA                 41
#define I2C_SCL                 40

char ssid[] = "MaxisONE Fibre 2.4G";
char pass[] = "respironics"; // leave this empty as this is an open network

String formattedDate;
String dayStamp;
String timeStamp;
String lastSyncRTC;

byte setH=0,setM=0,setS=0;
short int setYear=2023; 
byte setMonth=1, setDay=1;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Wire.begin(I2C_SDA, I2C_SCL);

  Serial.printf("\n\nConnecting to %s\n\n", ssid);

  WiFi.begin(ssid, pass); // normal connect method
  int wl_count = 0;
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print("#");
    wl_count++;
    delay(100);

    if(wl_count>50 && (WiFi.status() != WL_CONNECTED)){
      Serial.print("\nUnable to connect to network. Proceed in offline mode\n");
      delay(1000);
      break;
    }

  }

  if(WiFi.status() == WL_CONNECTED){
    Serial.println("WiFi Connected!");
    delay(2000);
    Serial.println("Syncing RTC via NTP now...");
    delay(2000);
    timeClient.begin();
    timeClient.setTimeOffset(28800);
    timeClient.update();
    sync_rtc_ntp();
    Serial.println("Syncing RTC via NTP complete. RTC is now up-to-date!");
    delay(2000);
  }


}

void loop() {
  // Serial.println(get_timestamp());
  get_rtc(); 
  delay(1000);
}

void get_rtc(){
  _clock.getTime();
  String hour = String(_clock.hour, DEC);  
  String min  = String(_clock.minute, DEC);
  String sec  = String(_clock.second, DEC);

  byte hourByte = hour.toInt();
  byte minByte  = min.toInt();
  byte secByte  = sec.toInt();

  String year = String(_clock.year+2000, DEC);
  String month = String(_clock.month, DEC);
  String day = String(_clock.dayOfMonth, DEC);

  int year_int = year.toInt();
  byte monthByte = month.toInt();
  byte dayByte = day.toInt();
  if(year_int < 2023 || year_int > 2050){
    Serial.println("Error. Date is invalid. Restarting now...");
    delay(1000);
    ESP.restart();
  }

  // Serial.print(_clock.hour, DEC);
  // Serial.print(":");
  // Serial.print(_clock.minute, DEC);
  // Serial.print(":");
  // Serial.print(_clock.second, DEC);
  // Serial.print("	");
  // Serial.printf("h_str:%s m_str:%s s_str:%s \n", hour, min, sec);

  Serial.printf("H_byte:%d M_byte:%d S_byte:%d\n", hourByte, minByte, secByte);
  Serial.printf("Y_int:%d Mth_byte:%d D_byte:%d\n\n", year_int, monthByte, dayByte);

}

void sync_rtc_ntp(){
  timeClient.update();
  setH = timeClient.getHours();
  setM = timeClient.getMinutes();
  setS = timeClient.getSeconds();  
  formattedDate   = timeClient.getFormattedDate(); 
  int splitT      = formattedDate.indexOf("T");
  dayStamp        = formattedDate.substring(0, splitT); // default to 4 || 2023-04-17
  setYear         = formattedDate.substring(0,4).toInt();
  setMonth        = formattedDate.substring(5,7).toInt();
  setDay          = formattedDate.substring(8,10).toInt();

  _clock.begin();
  _clock.fillByYMD(setYear,setMonth,setDay);
  _clock.fillByHMS(setH,setM,setS);
  _clock.fillDayOfWeek(setDay);
  _clock.setTime(); // write time and date to the RTC chip
  // Serial.printf("H:%d M:%d S:%d\n", setH, setM, setS);
  // Serial.printf("Year:%d Month:%d Day:%d\n", setYear, setMonth, setDay);
  
}

String get_timestamp(){
  timeClient.update();
  // https://github.com/taranais/NTPClient.git
  formattedDate   = timeClient.getFormattedDate(); 
  int splitT      = formattedDate.indexOf("T");
  dayStamp        = formattedDate.substring(0, splitT); // default to 4 || 2023-04-17
  String dateTime = timeClient.getFormattedTime() + " " + dayStamp;
  return dateTime;
}