/*
Testing UiTM WiFi IoT 9/12/2022 Friday
Works with ESP32S2
Edited 28 Dec 2022
Subscribed to Blynk Plus RM30.90/month on Monday 13 Feb 2023
Connected and Disconnected logic furnished 15 Feb 2023
*/
// #define ESP32S2_1
// #define ESP32S2_2
// #define ESP32S2_3
// #define ESP32S2_4
// #define ESP32S2_5
// #define ESP32S2_6
// #define ESP32C3_4
// #define ESP32DEV_1
#define ESP32DEV_2


/* Comment this out to disable prints and save space */
#define BLYNK_PRINT Serial
#define BLYNK_TEMPLATE_ID   "TMPLjsD8y_SC" // template ID for M04 and M05 then subcribing to Plus RM30.90 13Feb2023
#define BLYNK_TEMPLATE_NAME "AASAS UiTM WiFi IoT"

#ifdef ESP32S2_1
  #define BLYNK_DEVICE_NAME "AASAS M01"
  #define BLYNK_AUTH_TOKEN "J3DXwnJNxoCIUI3TF7ULHmCKdDg27FV4"
#elif defined ESP32S2_2
  #define BLYNK_DEVICE_NAME "AASAS M02"
  #define BLYNK_AUTH_TOKEN "AgRl8rHXRFkz4KnUvFWfQtlUCGZ6g8ug"
#elif defined ESP32S2_3
  #define BLYNK_DEVICE_NAME "AASAS M03"
  #define BLYNK_AUTH_TOKEN "vdnuDWozgCa3oP3_xXnekRTLmlo2mjuk"
#elif defined ESP32S2_4
  #define BLYNK_DEVICE_NAME "AASAS M04"
  #define BLYNK_AUTH_TOKEN "gee5lkJxSCmQrqplsAiH-uVPuNkF-B3G"
#elif defined ESP32S2_5
  #define BLYNK_DEVICE_NAME "AASAS M05"
  #define BLYNK_AUTH_TOKEN "Rq548So3QmWpZIJyAt59TVmW8W4GGlUd"
#elif defined ESP32S2_6
  #define BLYNK_DEVICE_NAME "AASAS M06"
  #define BLYNK_AUTH_TOKEN "qtER7nxNJH1QioLqmH_rE688VdJ5i0L0"
#elif defined ESP32DEV_1
  #define BLYNK_DEVICE_NAME "AASAS M07"
  #define BLYNK_AUTH_TOKEN "K-NDkXmOkZNC3TIKZo6EOrqdQQ8-Mr_f"
#elif defined ESP32DEV_2
  #define BLYNK_DEVICE_NAME "AASAS M08"
  #define BLYNK_AUTH_TOKEN "VoVZgVSmyKThZyh8KSS9oNq7gEcPLk0b"
#endif

#include "GroveBase-ESPDuino32-Mapping.h"

#include <Wire.h>
#include<ADS1115_WE.h> 
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
#include <HTTPClient.h>
#include <Arduino_JSON.h>
#include "DS1307.h"
#include <ChainableLED.h>


#define RGB         18 
#define NUMPIXELS   1 
#define DELAYVAL    100 

#if defined (ESP32DEV_1) || defined (ESP32DEV_2)  
  #define IN1         GROVE_D2
  #define IN2         GROVE_D3
  #define IN3         GROVE_D4
  #define LED_BLUE    GROVE_D5
  #define NUM_LEDS              1
  ChainableLED                  leds(GROVE_A0, GROVE_A1, NUM_LEDS); // (LEAVE A1 EMPTY)
  byte                          i = 0; // CHAINABLE LED ARRAY
  const int trig_pin = GROVE_D6; // D7 default
  const int echo_pin = GROVE_D7; // D6 default
#else
  #define IN1         19
  #define IN2         20
  #define IN3         21
  #define LED_BLUE    26  // assumption IO26 on ESP32S2
  #define NUM_LEDS              1
  ChainableLED                  leds(35, 36, NUM_LEDS); // (LEAVE A1 EMPTY) Dummy 35,36 on ESP32S2
  byte                          i = 0; // CHAINABLE LED ARRAY
  const int trig_pin = 33; // Dummy 33 on ESP32S2
  const int echo_pin = 34; // Dummy 34 on ESP32S2
#endif

#ifdef ESP32C3
  #define ONBOARD_LED 18
  #define R 3
  #define G 4
  #define B 5
#else
  #define ONBOARD_LED 2
#endif

// #include <math.h>


// Sound speed in air
#define SOUND_SPEED 340
#define TRIG_PULSE_DURATION_US 10
long ultrason_duration;
float distance_cm;
int waterLvlPercent;

const int B = 4275;               // B value of the thermistor
const int R0 = 100000;            // R0 = 100k
#if defined (ESP32DEV_1) || defined (ESP32DEV_2)  
const int pinTempSensor = GROVE_A3;
#else
const int pinTempSensor = 1;     // Grove - Temperature Sensor connect to IO0
#endif


char auth[] = BLYNK_AUTH_TOKEN;
// char ssid[] = "UiTM WiFi IoT";
// char pass[] = ""; // leave this empty as this is an open network

// char ssid[] = "Robotronix MKE2";
// char pass[] = "robotronix"; // leave this empty as this is an open network

char ssid[] = "MaxisONE Fibre 2.4G";
char pass[] = "respironics"; // leave this empty as this is an open network

// char ssid[] = "Maxis Postpaid 128";
// char pass[] = "respironics"; // leave this empty as this is an open network

#ifdef ESP32C3_4
  #define I2C_SDA                 8 
  #define I2C_SCL                 9
#elif defined ESP32DEV_1 || defined (ESP32DEV_2)
  // use default i2c pinout
#else
  #define I2C_SDA                 41 
  #define I2C_SCL                 40
#endif

#define wifiConnRetryAddress    0x0C // 12 & 13 : 2 bytes 
#define restartCounterAddress   0x0F // 15 & 16 : 2 bytes F 10 11
#define disconnCounterAddress   0x12 // 18 & 19 bytes
#define disconnectTS__Address   0x14 // 21 and beyond 
#define ADS_I2C_ADDRESS         0x48

WiFiUDP             ntpUDP;
NTPClient           timeClient(ntpUDP);
BlynkTimer          timer;
Adafruit_NeoPixel   pixels(NUMPIXELS, RGB);
Adafruit_INA219     ina219_A;
rgb_lcd             lcd;
ADS1115_WE          adc = ADS1115_WE(ADS_I2C_ADDRESS);
DS1307              _clock;

// Variables to save date and time
String formattedDate;
String dayStamp;
String timeStamp;
String restart_ts = "None";
String disconnected_ts = "None";
String hostname = BLYNK_DEVICE_NAME;
String default_hostname = "None";

float shuntvoltage;
float busvoltage;
float current_mA;

bool GROVE_LCD_AVAILABLE = false; // true for debugging purpose 27.02.2023 | change to false 03.03.2023
bool INA219_AVAILABLE = false;
bool ADS1115_AVAILABLE = false;

#define EEPROM_SIZE             48
#define FAST_DELAY              1000

int restartCounter;      // value will be loaded from EEPROM
int disconnection_count; // value will be loaded from EEPROM
int wifiRetryCount = -1 ; // value will be loaded from EEPROM

byte tick = 0;
byte red, green, blue;

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
BLYNK_DISCONNECTED();
BLYNK_RESTART();

BLYNK_WRITE(V20);
BLYNK_WRITE(V21);
BLYNK_WRITE(V22);
BLYNK_WRITE(V23);
BLYNK_WRITE(V24);
BLYNK_WRITE(V25);
BLYNK_WRITE(V26);

void BLYNK_TASK();
void i2c_scan();
void initINA219();
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
void clear_disconnCounter();
void try_wifi_connect(int timeout);

void onRelay1();
void onRelay2();
void onRelay3();
void offRelay1();
void offRelay2();
void offRelay3();
String get_timestamp();

bool openNetwork = false;

String openWeatherMapApiKey = "dbd7235bcc77e5896c73e975d013debe";
String city = "Kuching";
String countryCode = "MY";
unsigned long lastTime = 0;
unsigned long interval = 2000;

String jsonBuffer   = "None";
String jsonString   = "None";
String jsonHumid    = "None";
String jsonWeather  = "None";

float tempK ; 
float tempC ;
float humid ; 

String mac_str = "UNKNOWN MAC";
String esp_model = "UNKNOWN ESP";

void setup()
{
  Serial.begin(115200);
  #if defined (ESP32DEV_1) || defined (ESP32DEV_2)  
    Wire.begin();
  #else
    Wire.begin(I2C_SDA, I2C_SCL);
  #endif
  mac_str = WiFi.macAddress();
  const char* mac_addr = mac_str.c_str();
  Serial.print("MAC (String):");
  Serial.println(mac_str);
  Serial.printf("MAC (const char): %s\n", mac_addr);

  if(strcmp(mac_addr,"7C:DF:A1:AF:AA:B4")==0) esp_model = "ESP32C3-1";
  if(strcmp(mac_addr,"7C:DF:A1:AF:AC:FC")==0) esp_model = "ESP32C3-2";
  if(strcmp(mac_addr,"7C:DF:A1:AF:AB:00")==0) esp_model = "ESP32C3-3";
  if(strcmp(mac_addr,"7C:DF:A1:AF:AD:24")==0) esp_model = "ESP32C3-4"; // aziz
  if(strcmp(mac_addr,"7C:DF:A1:AF:AB:C0")==0) esp_model = "ESP32C3-5"; // aziz

  if(strcmp(mac_addr,"7C:DF:A1:00:72:C6")==0) esp_model = "ESP32S2-1";
  if(strcmp(mac_addr,"7C:DF:A1:00:BB:7C")==0) esp_model = "ESP32S2-2";
  if(strcmp(mac_addr,"7C:DF:A1:00:BA:9E")==0) esp_model = "ESP32S2-3";
  if(strcmp(mac_addr,"7C:DF:A1:00:6D:AA")==0) esp_model = "ESP32S2-4";
  if(strcmp(mac_addr,"7C:DF:A1:00:8D:74")==0) esp_model = "ESP32S2-5";
  if(strcmp(mac_addr,"7C:DF:A1:00:BA:BE")==0) esp_model = "ESP32S2-6";
  if(strcmp(mac_addr,"7C:DF:A1:00:A7:0C")==0) esp_model = "ESP32S2-7";
  if(strcmp(mac_addr,"7C:DF:A1:00:A7:38")==0) esp_model = "ESP32S2-8";

  if(strcmp(mac_addr,"C8:2B:96:B9:A9:58")==0) esp_model = "ESP32DEV1";
  if(strcmp(mac_addr,"9C:9C:1F:E3:85:3C")==0) esp_model = "ESP32DEV2";
  if(strcmp(mac_addr,"84:CC:A8:5E:6E:E8")==0) esp_model = "ESP32DEV3";
  if(strcmp(mac_addr,"9C:9C:1F:C5:94:24")==0) esp_model = "ESP32DEV4";
  if(strcmp(mac_addr,"84:0D:8E:E2:D6:D8")==0) esp_model = "ESP32DEV5";


  Serial.printf("[setup] %s Found!\n",esp_model);
  Serial.printf("[setup] MAC: %s\n", mac_addr);
  mac_str.remove(2,1); // remove the first : from MAC 

  Serial.println("\nScanning ESP32s2 i2c port...");
  i2c_scan(); // this method discovered to be failed after the latest ESP32 core update 24.02.2023 (Friday)
  Serial.println("\nReinit i2c port");
  #if defined (ESP32DEV_1) || defined (ESP32DEV_2)  
    Wire.begin();
  #else
    Wire.begin(I2C_SDA, I2C_SCL);
  #endif  
  Wire.setClock(125000); // discovered 27.02.2023 at Berlian | this is the key line to solve the weird i2c scanning issue 03.03.2023
  
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
    lcd.print(esp_model);
    lcd.setCursor(0,1);
    lcd.print(mac_str);
    delay(2000);
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Hello "+ String(BLYNK_DEVICE_NAME));
    lcd.setCursor(0,1);
    lcd.print("Connecting WiFi");
    delay(1000);
    lcd.clear();
  }  

  if(ADS1115_AVAILABLE){
    if(!adc.init()){
      Serial.println("ADS1115 not connected!");
    }
    adc.setVoltageRange_mV(ADS1115_RANGE_4096); //comment line/change parameter to change range
  }
  
  init_eeprom();
  // write16bitIntoEEPROM(wifiConnRetryAddress, 0); // comment this after debugging || the first upload

  // restartCounter = EEPROM.read(restartCounterAddress);
  restartCounter = read16bitFromEEPROM(restartCounterAddress);
  disconnection_count = read16bitFromEEPROM(disconnCounterAddress);
  wifiRetryCount = read16bitFromEEPROM(wifiConnRetryAddress);
  if(wifiRetryCount > 65535) wifiRetryCount = 0;
  check_restart_count();

  pinMode(ONBOARD_LED, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(LED_BLUE, OUTPUT);
  pinMode(trig_pin, OUTPUT); // We configure the trig as output
  pinMode(echo_pin, INPUT); // We configure the echo as input

  #ifdef ESP32C3
    pinMode(R,OUTPUT);
    pinMode(G,OUTPUT);
    pinMode(B,OUTPUT);
  #endif
  off_onboard_led();
  delay(1000);

  // WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE, INADDR_NONE);
  // Serial.print("ESP32 hostname before calling setting WiFi_STA: ");
  // Serial.println(default_hostname);   /*New Hostname printed*/ 

  WiFi.mode(WIFI_STA); //Optional
  default_hostname = WiFi.getHostname();

  // WiFi.setHostname(hostname.c_str());
  // Serial.print("ESP32 hostname after changing hostname: ");
  Serial.print("ESP32 hostname after calling setting WiFi_STA: ");
  Serial.println(default_hostname);   /*New Hostname printed*/ 

  if(GROVE_LCD_AVAILABLE){
    try_wifi_connect(10); // pre-connection using WiFi.begin() with 10 second default timeout
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Connecting Blynk");
    lcd.setCursor(0,1);
    lcd.write(1); // add arrow before SSID
    lcd.print(ssid);
  }

  if(GROVE_LCD_AVAILABLE){
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Init TimeClient!");
    lcd.setCursor(0,1);
    lcd.print("-Get restart ts!");
  }
  // 30 Dec 2022 Friday | Polished 10 Feb 2023 Friday
  // /Users/zidz/Documents/Arduino/libraries/Blynk/src/Adapters/BlynkArduinoClient.h (to fix connecting to blynk.cloud:80 infinite loop)
  // /Users/zidz/Documents/Arduino/libraries/Blynk/src/Blynk/BlynkProtocol.h (to edit logo)
  // /Users/zidz/Documents/Arduino/libraries/Blynk/src/BlynkSimpleEsp32.h (to edit Blynk.begin method)
  // /Users/zidz/Documents/Arduino/libraries/Blynk/src/Blynk/BlynkHandlers.h (BLYNK define methods)
  Serial.print("ESP32 NEW HOSTNAME: ");
  Serial.println(default_hostname);   /*New Hostname printed*/ 
  
  timeClient.begin();
  timeClient.setTimeOffset(28800);
  restart_ts = get_timestamp(); // must be called after timeClient.begin() for the restart timestamp

  byte setH=0,setM=0,setS=0;
  short int setYear=2023; 
  byte setMonth=1, setDay=1;

  setH = timeClient.getHours();
  setM = timeClient.getMinutes();
  setS = timeClient.getSeconds();

  formattedDate   = timeClient.getFormattedDate(); 
  int splitT      = formattedDate.indexOf("T");
  String date_only = formattedDate.substring(4, splitT);
  Serial.println(date_only);


  _clock.begin();
  _clock.fillByYMD(setYear,setMonth,setDay);
  _clock.fillByHMS(setH,setM,setS);
  _clock.fillDayOfWeek(WED);
  _clock.setTime(); // write time and date to the RTC chip

  Blynk.begin(auth, ssid, pass); // connectWiFi in Blynk is active. meaning connecting twice

  if(GROVE_LCD_AVAILABLE){
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Blynk Connected!");
    lcd.setCursor(0,1);
    lcd.print("-"+String(BLYNK_DEVICE_NAME)+" INIT-");
  }
  on_onboard_led();
  pixels.begin(); 
  loopRGB();
  rgbOff();
 
  timer.setInterval(1000L, BLYNK_TASK);
  timer.setInterval(1000L, get_weather);
}


void loop()
{
  Blynk.run();
  timer.run();
}

void try_wifi_connect(int timeout){
    wifiRetryCount++;
    write16bitIntoEEPROM(wifiConnRetryAddress, wifiRetryCount);

    int init_index_right = 9; // init 8 + 1 offset
    int init_index_left = 6; // init 7 - 1 offset
    int column_index_right = init_index_right; 
    int column_index_left = init_index_left;

    int wl_count = 0;
    int connect_elapse = timeout; // timeout in second unit 
    Serial.printf("\n\nConnecting to %s\n\n", ssid);
    if(strcmp(pass,"")==0){
      Serial.println("This is an open network!");
      openNetwork = true;
    }else{
      Serial.println("This is a close network!");
      openNetwork = false;
    }
    WiFi.begin(ssid, pass); // normal connect method
    lcd.setCursor(0, 0); // row 1, column 0
    lcd.print("Conn Retry:" +String(wifiRetryCount));  
    while (WiFi.status() != WL_CONNECTED) {
          tick++;
          Serial.printf("/");
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
                lcd.print("Conn Retry:" +String(wifiRetryCount));  
                lcd.setCursor(0, 1); // row 1, column 0
                lcd.print(default_hostname);
                delay(1000);
                off_onboard_led();

            }
            else if(millis()/1000 % 3 == 0 || millis()/1000 % 5 == 0){
                lcd.clear();
                lcd.setCursor(0, 0); // row 1, column 0
                lcd.print("Connecting WiFi");
                lcd.setCursor(0, 1); // row 1, column 0
                if(openNetwork) lcd.print("    Open Network");  
                else            lcd.print("   Close Network");  
                delay(1000);
            }
            else{
                lcd.setCursor(0, 0); // row 1, column 0
                lcd.print(String(ssid));
                on_onboard_led();
            }
          }

      wl_count++;
      delay(100);
      // vTaskDelay(100 / portTICK_PERIOD_MS);
      
      if(wl_count > connect_elapse*10 && (WiFi.status() != WL_CONNECTED)){
          Serial.println("\n\n [ESP32_AASAS_UiTM_WiFi_IoT] Cant join network within timeout. Restarting...");
          wl_count = 0;
          lcd.clear();
          lcd.setCursor(0, 0); // row 1, column 0
          lcd.print("Not Connected!");
          lcd.setCursor(0, 1); // row 1, column 0
          lcd.print("Restarting ESP32");
          delay(1000);
          ESP.restart();
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
   
    if(GROVE_LCD_AVAILABLE){
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("-WiFi Connected-");
      lcd.setCursor(0,1);
      lcd.print("--UiTM WiFi IoT-");
      delay(1000);
      // vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
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
  Serial.printf("H_byte:%d M_byte:%d S_byte:%d --- ", hourByte, minByte, secByte);
  Serial.printf("Y_int:%d Mth_byte:%d D_byte:%d\n", year_int, monthByte, dayByte);

}

void sync_rtc_ntp(){
  String formattedDate;
  String dayStamp;
  String timeStamp;
  String lastSyncRTC;

  byte setH=0,setM=0,setS=0;
  short int setYear=2023; 
  byte setMonth=1, setDay=1;

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
  dayStamp        = formattedDate.substring(5, splitT);
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
  digitalWrite(LED_BLUE,1);
}
void offRelay1(){
  digitalWrite(IN1,0);
  digitalWrite(LED_BLUE,0);
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


void setRGB_from_RSSI(int rssi){

  /*
  good segment
  if(rssi > -30) out = "Perfect signal";  // 0 -> -30
  else if(rssi > -50) out = "Excellent signal"; -30 -> -50
  
  average segment
  else if(rssi > -60) out = "Good reliable signal"; -50 -> -60
  else if(rssi > -67) out = "Voice and Non-HD vid"; -60 -> -67
  else if(rssi > -70) out = "Light browsing and email"; -67 -> -70

  bad segment
  else if(rssi > -80) out = "Unstable connection"; -70 -> -80
  else                out = "Unlikely connection"; -80 -> -120

  */
  if(rssi < -70){
    red = map(rssi,-120,-69,255,128); // towards red
    // green = map(red,128,255,128,0); // change according to red for transition 
    green = 0;   
    blue = 0; // stay off
  }
  else if(rssi >= -70 && rssi < -50){
    green = 255; // towards green    
  }
  else{
    red = 0; // stay off
    blue = map(rssi,-50,0,128,255); // towards blue
    green = map(blue,128,255,128,0); // change according to blue for transition    
  }
    
}

void mapRGBtoRSSI(byte r, byte g, byte b){
  #ifndef ESP32C3
    pixels.setPixelColor(0, pixels.Color(r, g, b));
    pixels.show(); 
  #endif
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

void write16bitIntoEEPROM(int address, int number)
{ 
  EEPROM.write(address, number >> 8);
  EEPROM.write(address + 1, number & 0xFF);
  EEPROM.commit();
  delay(5);
}

int read16bitFromEEPROM(int address)
{
  return (EEPROM.read(address) << 8) + EEPROM.read(address + 1);
}

void writeStringToEEPROM(int addrOffset, const String &strToWrite)
{
  byte len = strToWrite.length();
  EEPROM.write(addrOffset, len);

  for (int i = 0; i < len; i++)
  {
    EEPROM.write(addrOffset + 1 + i, strToWrite[i]);
  }
  EEPROM.commit();
  delay(5);

}

String readStringFromEEPROM(int addrOffset)
{
  int newStrLen = EEPROM.read(addrOffset);
  char data[newStrLen + 1];

  for (int i = 0; i < newStrLen; i++)
  {
    data[i] = EEPROM.read(addrOffset + 1 + i);
  }
  data[newStrLen] = '\0'; // !!! NOTE !!! Remove the space between the slash "\" and "0" (I've added a space because otherwise there is a display bug)

  return String(data);
}

void clear_wifiRetryCounter(){
  if(GROVE_LCD_AVAILABLE){
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("Reset WiFi Retry!");
      lcd.setCursor(0,1);
      lcd.print("-Restarting Now-");
    }
    Serial.println("@@@@@@@@@@@@@@@@@@ CLEARING WiFi Retry Counter @@@@@@@@@@@@@@@@@@ ");
    write16bitIntoEEPROM(wifiConnRetryAddress,0);
}

void clear_disconnCounter(){
    if(GROVE_LCD_AVAILABLE){
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("-Reset Disconn!-");
      lcd.setCursor(0,1);
      lcd.print("-Restarting Now-");
    }
    Serial.println("@@@@@@@@@@@@@@@@@@ CLEARING Disconnection Counter @@@@@@@@@@@@@@@@@@ ");
    disconnection_count = 0; // reset the global value here 17 Feb Friday
    write16bitIntoEEPROM(disconnCounterAddress,disconnection_count);

    Blynk.virtualWrite(V15, "Cleared DC Counter");
    Blynk.virtualWrite(V16, 0); // disconnection counter reset to zero
    // EEPROM.write(restartCounterAddress, 255);

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
    write16bitIntoEEPROM(restartCounterAddress,0);
    // EEPROM.write(restartCounterAddress, 255);
    ESP.restart();
 
}

void check_restart_count(){
  if(restartCounter == 0){
      Serial.println("[INFO] New deployment node. Restart count is NULL");
      restartCounter += 1; // initial increment [compulsory]
      Serial.printf("[INFO] Initial restart count is ONE = %d\n", restartCounter);
      write16bitIntoEEPROM(restartCounterAddress,restartCounter);
  }
  else{
      Serial.println("[INFO] Normal reboot or restart");
      restartCounter += 1;  // increment rst count by 1 for each reboot
      Serial.printf("[INFO] the current restart count is %d\n", restartCounter);
      write16bitIntoEEPROM(restartCounterAddress,restartCounter);
  }
}

void init_eeprom(){

  if (!EEPROM.begin(EEPROM_SIZE))
  {
    Serial.println("failed to initialise EEPROM"); delay(1000000);
  }

  // Serial.println(" bytes read from Flash:");
  // for (int i = 0; i < EEPROM_SIZE; i++)
  // {
  //   if(i >= 21){
  //     Serial.print("Disconnected Timestamp String Memory "); Serial.print(i);   Serial.print(":");
  //   }
  //   // else if(i== 12 || i==13){
  //   //     Serial.print("WiFi Retry Count Memory "); Serial.print(i);   Serial.print(":");
  //   // }
  //   // else if(i== 15 || i==16){
  //   //     Serial.print("Restart Count Memory "); Serial.print(i);   Serial.print(":");
  //   // }else if (i== 18 || i==19){
  //   //     Serial.print("Disconnection Count Memory "); Serial.print(i);   Serial.print(":");
  //   // }else{
  //   //     Serial.print("R "); Serial.print(i);   Serial.print(":");
  //   // }
  //   Serial.print(byte(EEPROM.read(i))); Serial.println();
  // }
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
  byte error, address, nDevices = 0;
  Serial.println("Scanning...");
  // Wire.setClock(125000); // discovered 27.02.2023 at Berlian | disabled 3.03.2023 Friday MKE2 (issue fixed)
  for(address = 1; address < 127; address++ ) {
    Wire.beginTransmission(address);
    delay(1); // re-enabled 3.03.2023 Friday MKE2 (issue fixed) | kind of fixing the slow scanning issue 03.03.2023
    error = Wire.endTransmission();
    Serial.printf("Address: 0x%02x \t Error: %d\n", address, error); // added 27.02.2023 at Berlian

    if (error == 0) {
      Serial.print("------------------------------- I2C device found at address 0x");
      if (address<16) {
        Serial.print("0");
      }
      Serial.println(address,HEX);
      if(address == 0x3E) GROVE_LCD_AVAILABLE = true;
      if(address == 0x40) INA219_AVAILABLE    = true;
      if(address == 0x48) ADS1115_AVAILABLE   = true;
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
  }
  else {
    Serial.println("done\n");
  }
  // Wire.endTransmission(false); // added 03.03.2023 Friday MKE2 (issue fixed)
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

String get_rssi_state(int rssi){
  String out;
  if(rssi > -30) out = "Perfect signal";
  else if(rssi > -50) out = "Excellent signal";
  else if(rssi > -60) out = "Good reliable signal";
  else if(rssi > -67) out = "Voice and Non-HD vid";
  else if(rssi > -70) out = "Light browsing and email";
  else if(rssi > -80) out = "Unstable connection";
  else                out = "Unlikely connection";
  return out;
}

void get_weather(){
    String serverPath = "http://api.openweathermap.org/data/2.5/weather?q=" + city + "," + countryCode + "&APPID=" + openWeatherMapApiKey;
    
    jsonBuffer = httpGETRequest(serverPath.c_str());
    #ifdef DEBUG_SERIAL
    // Serial.println(jsonBuffer); // print all JSON output
    #endif
    JSONVar myObject = JSON.parse(jsonBuffer);

    // JSON.typeof(jsonVar) can be used to get the type of the var
    if (JSON.typeof(myObject) == "undefined") {
      #ifdef DEBUG_SERIAL
      Serial.println("Parsing input failed!");
      #endif
      return;
    }

    // 0K − 273.15

    jsonString = JSON.stringify(myObject["main"]["temp"]);
    jsonHumid = JSON.stringify(myObject["main"]["humidity"]);

    tempK = jsonString.toFloat(); 
    tempC = tempK - 273.15;
    humid = jsonHumid.toFloat(); 

    jsonWeather = JSON.stringify(myObject["weather"][0]["description"]);
    // String jsonWeather = "thunderstorm with light rain";

    #ifdef DEBUG_SERIAL
    Serial.print("JSON object = ");
    Serial.println(myObject);
    Serial.print("Temperature: ");
    Serial.println(tempC);
    Serial.print("Pressure: ");
    Serial.println(myObject["main"]["pressure"]);
    Serial.print("Humidity: ");
    Serial.println(myObject["main"]["humidity"]);
    Serial.print("Wind Speed: ");
    Serial.println(myObject["wind"]["speed"]);
    Serial.print("Weather: ");
    Serial.println(myObject["weather"][0]["description"]);
    Serial.print("Feels like: ");
    Serial.println(myObject["main"]["feels_like"]);
    #endif
}

String httpGETRequest(const char* serverName) {
  WiFiClient client;
  HTTPClient http;
    
  // Your Domain name with URL path or IP address with path
  http.begin(client, serverName);
  
  // Send HTTP POST request
  int httpResponseCode = http.GET();
  
  String payload = "{}"; 
  
  if (httpResponseCode>0) {
    #ifdef DEBUG_SERIAL
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    #endif
    payload = http.getString();
  }
  else {
    #ifdef DEBUG_SERIAL
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
    #endif

  }
  // Free resources
  http.end();

  return payload;
}


void read_ads1115(){
  float ch0 = readChannel(ADS1115_COMP_0_GND);   // turbidity sensor   
  float ch1 = readChannel(ADS1115_COMP_1_GND);   // V16 takeover from GPS char count
  float ch2 = readChannel(ADS1115_COMP_2_GND);   // V41 takeover from GPS fix age
  float ch3 = readChannel(ADS1115_COMP_3_GND);   // V27 takeover from Sat in Use
  String ads_readout = "ch0: "+ String(ch0)+" mV ch1: "+ String(ch1,0)+" mV ch2: "+ String(ch2)+" mV ch3: "+ String(ch3)+" mV";
  Blynk.virtualWrite(V18, ads_readout);
}

float readChannel(ADS1115_MUX channel) {
  float voltage = 0.0;
  adc.setCompareChannels(channel);
  adc.startSingleMeasurement();
  while(adc.isBusy()){}
  voltage = adc.getResult_mV(); // alternative: getResult_mV for Millivolt
  return voltage;
}

float get_ambient_temp()
{
    int a = analogRead(pinTempSensor);
    // int scaled = map(a,0,8192,0,1023);
    // float R = 1023.0/scaled-1.0; // 1023 = 1 ... 8192 = 8
    float R = 8191.0/a-8.0; 

    R = R0*R;

    float temperature = 8.0/(log(R/R0)/B+1/298.15)-273.15; // convert to temperature via datasheet
    // Serial.printf("analogRead at pin %d = %d and R value = %f\n", pinTempSensor, a, R);
    // Serial.print("temperature = ");
    // Serial.println(temperature);
    return temperature;
}
  
void BLYNK_TASK(){
    tick++; // to replace tick
    getINA219();
    if(ADS1115_AVAILABLE) 
      read_ads1115(); //   Blynk.virtualWrite(V18, ads_readout) happening inside func
    else{
      // Blynk.virtualWrite(V18, "ADS1115 Not Connected"); // before adding sonar SR04-M2
      Blynk.virtualWrite(V18, "Water Level = " + String(get_water_level_cm())+" cm @ " + String(waterLvlPercent)+"%");
    }
    if(tick % 3 == 0){
      on_onboard_led();
      if(busvoltage >= 13)
        setRGBtoWhite();
      else if(busvoltage >= 12 && busvoltage < 13)
        setRGBtoBlue();
      else if(busvoltage >= 11 && busvoltage < 12)
        setRGBtoGreen();
      else
        setRGBtoRed();
    }
    else{
      off_onboard_led();    
    }
    String dateTime = get_timestamp();
    // Serial.print("Datetime:");
    // Serial.println(dateTime);
    int RSSI_dBm =  WiFi.RSSI();
    setRGB_from_RSSI(RSSI_dBm);
    if(millis() % 2 == 0)
      mapRGBtoRSSI(red, green, blue);
    else
      rgbOff();
    
    Blynk.virtualWrite(V0,uptime_formatter::getUptime());
    Blynk.virtualWrite(V1, dateTime);
    Blynk.virtualWrite(V4, RSSI_dBm);
    Blynk.virtualWrite(V6, busvoltage);
    Blynk.virtualWrite(V7, current_mA); 
    Blynk.virtualWrite(V9, get_rssi_state(RSSI_dBm));
    Blynk.virtualWrite(V10, tempC);
    Blynk.virtualWrite(V11, humid);
    Blynk.virtualWrite(V12, jsonWeather);
    Blynk.virtualWrite(V34, get_ambient_temp());

    if(GROVE_LCD_AVAILABLE)
      lcd.clear();

    if(tick % 2 == 0){
      if(GROVE_LCD_AVAILABLE){
        lcd.setCursor(0,0); // row 0, column 0
        lcd.print("----"+ String(BLYNK_DEVICE_NAME)+"---"); // ----AASAS ONE---
        lcd.setCursor(0,1);
        lcd.print(dateTime);
      }
    }
    else if(tick % 5 == 0){
      if(GROVE_LCD_AVAILABLE){
        lcd.setCursor(0,0); // row 0, column 0
        lcd.print(ssid); // print connected SSID
        lcd.setCursor(0,1);
        lcd.print(dateTime);
      }
    }
    else if(tick % 7 == 0){
      if(GROVE_LCD_AVAILABLE){
        lcd.setCursor(0,0); // row 0, column 0
        lcd.print(esp_model); // print connected SSID
        lcd.setCursor(0,1); // row 0, column 0
        lcd.print(mac_str); // print connected SSID
      }
    }
    else if(tick % 11 == 0){
      if(GROVE_LCD_AVAILABLE){
        lcd.setCursor(0,0); // row 0, column 0
        lcd.print("RSSI: "+String(RSSI_dBm)+" dBm"); // print connected SSID
        lcd.setCursor(0,1); // row 0, column 0
        lcd.print(get_rssi_state(RSSI_dBm)); // print connected SSID
      }
    }
    else{
      if(GROVE_LCD_AVAILABLE){
        display_uptime_top_row();
        lcd.setCursor(0,1); // row 0, column 0
        lcd.print(String(busvoltage)+"V I:"+String(current_mA)+"mA");
      }
    }    
    rgb_LED_Off();      
}

BLYNK_CONNECTED() {
  blueOn();
  if(GROVE_LCD_AVAILABLE){
    lcd.clear();
    lcd.setCursor(0,0); // row 0, column 0
    lcd.print("Blynk Connected!"); // print connected SSID
    lcd.setCursor(0,1); // row 0, column 0
    lcd.print("Device is Online"); // print connected SSID
    delay(1000);
  }
  Blynk.virtualWrite(V19, wifiRetryCount);
  wifiRetryCount = 0; // reset this back for the next cycle
  write16bitIntoEEPROM(wifiConnRetryAddress, wifiRetryCount);
  String blynk_on_connected_ts = get_timestamp();
  Blynk.syncVirtual(V20,V21,V22,V23,V24,V25,V26);
  Blynk.virtualWrite(V2, WiFi.localIP().toString());
  Blynk.virtualWrite(V3, restart_ts); 
  Blynk.virtualWrite(V5, restartCounter);
  Blynk.virtualWrite(V8, ssid);
  String disconn_ts_str = readStringFromEEPROM(disconnectTS__Address);
  Serial.print("The disconnection timestamp from EEPROM: ");
  Serial.println(disconn_ts_str);
  if(disconnection_count == 0)
    Blynk.virtualWrite(V15, "Not yet");
  else
    Blynk.virtualWrite(V15, disconn_ts_str);
  disconnection_count = read16bitFromEEPROM(disconnCounterAddress);
  Blynk.virtualWrite(V16, disconnection_count);
  Blynk.virtualWrite(V17, blynk_on_connected_ts); 
  Blynk.virtualWrite(V29, default_hostname); 
}

BLYNK_DISCONNECTED() {
    redOn();
    disconnection_count++;
    write16bitIntoEEPROM(disconnCounterAddress,disconnection_count);
    disconnected_ts = get_timestamp();
    writeStringToEEPROM(disconnectTS__Address, disconnected_ts);
  
    Serial.println("[ESP32_AASAS_UiTM_WiFi_IoT] Blynk Disconnected");
    if(GROVE_LCD_AVAILABLE){
      lcd.clear();
      lcd.setCursor(0,0); // row 0, column 0
      lcd.print("Blynk Disconnected"); // print connected SSID
      lcd.setCursor(0,1); // row 0, column 0
      lcd.print("WiFi is Down"); // print connected SSID
      delay(1000);
      lcd.clear();
      lcd.setCursor(0,0); // row 0, column 0
      lcd.print("Reinit Blynk"); // print connected SSID
      lcd.setCursor(0,1); // row 0, column 0
      lcd.print("> Blynk.begin()"); // print connected SSID
      delay(1000);
    }
    Blynk.begin(auth, ssid, pass); // must have logic to restart handshake with Blynk 10 Feb 2023
}

BLYNK_RESTART(){
  if(GROVE_LCD_AVAILABLE){
    lcd.clear();
    lcd.setCursor(0,0); // row 0, column 0
    lcd.print("-Blynk Restart!-"); // print connected SSID
    lcd.setCursor(0,1); // row 0, column 0
    lcd.print("-Force Restart!-"); // print connected SSID
    delay(1000);
  }
  disconnection_count = 0; // reset the global value here 17 Feb Friday
  // this is essential so that at every restart cycle disconnection counter will revert to zero
  // this means disconnection counter will only be incremented when the device has not gone into cold restart
  // disconnection counter will be incremented during current restart session only via BLYNK_DISCONNECTED and BLYNK_CONNECTED
  write16bitIntoEEPROM(disconnCounterAddress,disconnection_count);
}

BLYNK_ELAPSE_OUT(char state, int val){
/*
 state:
 c = connect wifi
 r = reconnect wifi
 w = wait to restart
*/
  if(GROVE_LCD_AVAILABLE){
    lcd.clear();
    lcd.setCursor(0,0); // row 0, column 0
    if(state == 'c') 
      lcd.print("[B]Connect WiFi"); // print connected SSID
    else if(state == 'r')
      lcd.print("[B]Reconnecting"); // print connected SSID
    else
      lcd.print("[B]Restart in...");
    lcd.setCursor(0,1); // row 0, column 0
    lcd.print("[B]Elapse: "+String(val)+" s"); // print connected SSID
  }
  // Serial.println("[ESP32_AASAS_UiTM_WiFi_IoT] Elapse out here");
  // Serial.println(val);
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

BLYNK_WRITE(V27){
  int pinValue = param.asInt();
  if(pinValue) clear_disconnCounter();
}

BLYNK_WRITE(V28){
  int pinValue = param.asInt();
  if(pinValue) clear_wifiRetryCounter();
}


void cycleRGB(){
  setRGBtoRed();
  vTaskDelay(250 / portTICK_PERIOD_MS);
  setRGBtoGreen();  
  vTaskDelay(250 / portTICK_PERIOD_MS);
  setRGBtoBlue(); 
  vTaskDelay(250 / portTICK_PERIOD_MS);
}
void rgb_LED_Off(){
  leds.setColorRGB(i, 0, 0, 0);
}
void setRGBtoWhite(){
  leds.setColorRGB(i, 255, 255, 255);
}

void setRGBtoRed(){
  leds.setColorRGB(i, 255, 0, 0);
}

void setRGBtoGreen(){
  leds.setColorRGB(i, 0, 255, 0);
}

void setRGBtoBlue(){
  leds.setColorRGB(i, 0, 0, 255);
}

void mapRGBtoPH(byte r, byte g, byte b){
  leds.setColorRGB(i, r, g, b);
}

float get_water_level_cm() {
  // Set up the signal
  digitalWrite(trig_pin, LOW);
  delayMicroseconds(2);
 // Create a 10 µs impulse
  digitalWrite(trig_pin, HIGH);
  delayMicroseconds(TRIG_PULSE_DURATION_US);
  digitalWrite(trig_pin, LOW);

  // Return the wave propagation time (in µs)
  ultrason_duration = pulseIn(echo_pin, HIGH);

//distance calculation
  distance_cm = ultrason_duration * SOUND_SPEED/2 * 0.0001;
  waterLvlPercent = map(distance_cm,150,30,0,100);  

  // We print the distance on the serial port
  // Serial.print("Distance (cm): ");
  // Serial.println(distance_cm);
  return distance_cm;
}

