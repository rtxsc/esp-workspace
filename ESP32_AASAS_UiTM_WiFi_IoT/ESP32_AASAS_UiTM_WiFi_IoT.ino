/*
Testing UiTM WiFi IoT 9/12/2022 Friday
Works with ESP32S2
Edited 28 Dec 2022
Subscribed to Blynk Plus RM30.90/month on Monday 13 Feb 2023
Connected and Disconnected logic furnished 15 Feb 2023

solving MD5 flash issue Sunday 18 June 2023 at TBS
python3 -m esptool --chip esp32 write_flash_status --non-volatile 0
python3 -m esptool --chip esp32 erase_flash 
*/
// #define ESP32S2_1  // M01
// #define ESP32S2_2  // M02
// #define ESP32S2_3  // M03
// #define ESP32S2_4  // M04
// #define ESP32S2_5  // M05
// #define ESP32S2_6  // M06
#define ESP32DEV_1 // M07
// #define ESP32DEV_2 // M08
// #define ESP32DEV_3
// #define ESP32DEV_4 // M09 missing usb port @ TBS front gate
// #define ESP32DEV_5 // M10
// #define ESP32DEV_6 // acting as M01 alongside ESP32S2_1
// #define ESP32C3_4

bool deep_sleep_activated = false;
// deep sleep config
#define uS_TO_S_FACTOR 1000000ULL  /* Conversion factor for micro seconds to seconds */
#define SLEEP_HOUR     14
#define TIME_TO_SLEEP  SLEEP_HOUR*3600       /* Time ESP32 will go to sleep (in seconds) 50400 */
int setSleepHour = 18;
int setSleepMin = 0;
uint8_t etaHour = 0;
uint8_t etaMin = 0;
uint8_t etaSec = 0;
String etaDS = "HH:MM:SS";
RTC_DATA_ATTR int bootCount = 0;

// #define REGULAR_I2C_LCD // comment if using GROVE_LCD

// #define LOCATION_MKE2_UiTM_WiFi_IoT // comment this line for TBS deployment
// #define LOCATION_MKE2_MaxisONE // comment this line for TBS deployment

/* Comment this out to disable prints and save space */
#define BLYNK_PRINT Serial
#define BLYNK_TEMPLATE_ID   "TMPLjsD8y_SC" // template ID for M04 and M05 then subcribing to Plus RM30.90 13Feb2023
#define BLYNK_TEMPLATE_NAME "AASAS UiTM WiFi IoT"

#if defined ESP32S2_1 || defined ESP32DEV_6
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
#elif defined ESP32DEV_4
  #define BLYNK_DEVICE_NAME "AASAS M09"
  #define BLYNK_AUTH_TOKEN "ZELuVGWY16O3KPW8mLkgkdISj2ohVgP7"
#elif defined ESP32DEV_5
  #define BLYNK_DEVICE_NAME "AASAS M10"
  #define BLYNK_AUTH_TOKEN "Wo6dEX9FFGRjR-fbRShY-FxM9uYAYdqp"
#endif

#include "GroveBase-ESPDuino32-Mapping.h"
#include <esp_now.h>
#include <Wire.h>
#include<ADS1115_WE.h> 
#include "rgb_lcd.h"
#include <LiquidCrystal_I2C.h>

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

TaskHandle_t Task1; // ESPNOW
TaskHandle_t Task2; // BLYNK

// MAC Address of the receiver #1 => 9c:9c:1f:c5:94:24 (ESP01-client-1)
uint8_t client1_mac[] = {0x9C, 0x9C, 0x1F, 0xC5, 0x94, 0x24};
String client1_mac_string = "9c:9c:1f:c5:94:24"; // must be lowercase 27.08.2022 Sat
const char* client1_cchar = client1_mac_string.c_str();

// MAC Address of the receiver #2 => 9c:9c:1f:e3:85:3c (ESP01-client-2)
uint8_t client2_mac[] = {0x9C, 0x9C, 0x1F, 0xE3, 0x85, 0x3C};
String client2_mac_string = "9c:9c:1f:e3:85:3c"; // must be lowercase 27.08.2022 Sat
const char* client2_cchar = client2_mac_string.c_str();

// MAC Address of the receiver #3  7C:DF:A1:AF:AA:B4 (ESP32-C3 26.08.2022)
// 7C:DF:A1:AF:AB:00 (changed 9 June 2023 after crosscheck)
uint8_t client3_mac[] = {0x7C, 0xDF, 0xA1, 0xAF, 0xAB, 0x00};
String client3_mac_string = "7c:df:a1:af:ab:00"; // must be lowercase 27.08.2022 Sat
const char* client3_cchar = client3_mac_string.c_str();

// 7C:DF:A1:AF:AD:24 (C3-4) added 10.06.2023 saturday after SKJMT IBIZ
uint8_t client4_mac[] = {0x7C, 0xDF, 0xA1, 0xAF, 0xAD, 0x24};
String client4_mac_string = "7c:df:a1:af:ad:24"; // new C3-74 added 9.june.2023
const char* client4_cchar = client4_mac_string.c_str();

// 7C:DF:A1:AF:AC:B8 (C3-6)
uint8_t client6_mac[] = {0x7C, 0xDF, 0xA1, 0xAF, 0xAC, 0xB8}; // MAC not confirmed 10.june.2023
String client6_mac_string = "7c:df:a1:af:ac:b8"; // MAC not confirmed 10.june.2023
const char* client6_cchar = client6_mac_string.c_str(); // changed 10.june.2023 after SKJMT


// 7C:DF:A1:AF:A8:D8(C3-7)
uint8_t client7_mac[] = {0x7C, 0xDF, 0xA1, 0xAF, 0xA8, 0xD8}; // MAC not confirmed 10.june.2023
String client7_mac_string = "7c:df:a1:af:a8:d8"; // MAC not confirmed 10.june.2023
const char* client7_cchar = client7_mac_string.c_str(); // changed 10.june.2023 after SKJMT


//7C:DF:A1:AF:AD:20 (C3-8)
uint8_t client8_mac[] = {0x7C, 0xDF, 0xA1, 0xAF, 0xAD, 0x20}; // changed 10.june.2023 after SKJMT
String client8_mac_string = "7c:df:a1:af:ad:20"; // new C3-7 added 9.june.2023
const char* client8_cchar = client8_mac_string.c_str(); // changed 10.june.2023 after SKJMT

// Structure example to receive data
// Must EXACTLY MATCH THE PATTERN of the sender's structure
typedef struct struct_message {
  String id;
  unsigned int readingId;
  float volt;
  float amps;
  float temp;
  float humi;
  float mois;
  float rain;
} struct_message;

struct_message incomingReadings;

//Structure example to send data
//Must match the receiver structure (client-1 and client-2)
typedef struct struct_control {
    int control;
} struct_control;


//Create a struct_message called sendControl
struct_control sendControl;
JSONVar     board;

bool esp_now_initialized = false;
#define CORE_0      0x00
#define CORE_1      0x01

bool displayESPNOW = false;
String jsonString1 = "None";
String dt1 = "None";
float temp1 = 0;
float humi1 = 0;
float moist1 = 0;
float rain1 = 0;
float volt1 = 0;
float amps1 = 0;
uint16_t read_id1 = 0;
uint16_t prev_read_id1 = 0;

String jsonString2 = "None";
String dt2 = "None";
float temp2 = 0;
float humi2 = 0;
float moist2 = 0;
float rain2 = 0;
float volt2 = 0;
float amps2 = 0;
uint16_t read_id2 = 0;
uint16_t prev_read_id2 = 0;

String jsonString3 = "None";
String dt3 = "None";
float temp3 = 0;
float humi3 = 0;
float moist3 = 0;
float rain3 = 0;
float volt3 = 0;
float amps3 = 0;
uint16_t read_id3 = 0;
uint16_t prev_read_id3 = 0;

String jsonString4 = "None";
String dt4 = "None";
float temp4 = 0;
float humi4 = 0;
float moist4 = 0;
float rain4 = 0;
float volt4 = 0;
float amps4 = 0;
uint16_t read_id4 = 0;
uint16_t prev_read_id4 = 0;

String jsonString6 = "None";
String dt6 = "None";
float temp6 = 0;
float humi6 = 0;
float moist6 = 0;
float rain6 = 0;
float volt6 = 0;
float amps6 = 0;
uint16_t read_id6 = 0;
uint16_t prev_read_id6 = 0;

String jsonString7 = "None";
String dt7 = "None";
float temp7 = 0;
float humi7 = 0;
float moist7 = 0;
float rain7 = 0;
float volt7 = 0;
float amps7 = 0;
uint16_t read_id7 = 0;
uint16_t prev_read_id7 = 0;

String jsonString8 = "None";
String dt8 = "None";
float temp8 = 0;
float humi8 = 0;
float moist8 = 0;
float rain8 = 0;
float volt8 = 0;
float amps8 = 0;
uint16_t read_id8 = 0;
uint16_t prev_read_id8 = 0;

#define RGB         18 
#define NUMPIXELS   1 
#define DELAYVAL    100 

#if defined (ESP32DEV_1) || defined (ESP32DEV_2) || defined (ESP32DEV_3) || defined (ESP32DEV_4) || defined (ESP32DEV_5) || defined (ESP32DEV_6)     
  #define IN1         GROVE_D2
  #define IN2         GROVE_D3
  #define IN3         GROVE_D4
  #define LED_BLUE    GROVE_D5
  #define NUM_LEDS    1
  ChainableLED        leds(GROVE_A0, GROVE_A1, NUM_LEDS); // (LEAVE A1 EMPTY)
  byte                i = 0; // CHAINABLE LED ARRAY
  const int trig_pin = GROVE_D6; // D7 default
  const int echo_pin = GROVE_D7; // D6 default
  #define BLINKER     GROVE_D13
#else
  #define IN1         19
  #define IN2         20
  #define IN3         21
  #define LED_BLUE    26  // assumption IO26 on ESP32S2
  #define NUM_LEDS     1
  ChainableLED        leds(35, 36, NUM_LEDS); // (LEAVE A1 EMPTY) Dummy 35,36 on ESP32S2
  byte                i = 0; // CHAINABLE LED ARRAY
  const int trig_pin = 33; // Dummy 33 on ESP32S2
  const int echo_pin = 34; // Dummy 34 on ESP32S2
  #define BLINKER     37 // Dummy

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
#if defined (ESP32DEV_1) || defined (ESP32DEV_2) || defined (ESP32DEV_3) || defined (ESP32DEV_4) || defined (ESP32DEV_5) || defined (ESP32DEV_6)       
const int pinTempSensor = GROVE_A3;
#else
const int pinTempSensor = 1;     // Grove - Temperature Sensor connect to IO0
#endif


char auth[] = BLYNK_AUTH_TOKEN;

#ifdef LOCATION_MKE2_UiTM_WiFi_IoT
  char ssid[] = "UiTM WiFi IoT";
  char pass[] = ""; // leave this empty as this is an open network
#elif defined LOCATION_MKE2_MaxisONE
  char ssid[] = "MaxisONE Fibre 2.4G";
  char pass[] = "respironics"; // leave this empty as this is an open network
#else
  char ssid[] = "MaxisONE Fibre 2.4G_EXT"; // TP-link extender / DEV-4 doesnt need EXT
  char pass[] = "respironics"; // leave this empty as this is an open network
#endif

String ssidstr = "None";
// char ssid[] = "Robotronix MKE2";
// char pass[] = "robotronix"; // leave this empty as this is an open network

// char ssid[] = "Maxis Postpaid 128";
// char pass[] = "respironics"; // leave this empty as this is an open network

#ifdef ESP32C3_4 || defined (ESP32C3_6) || defined (ESP32C3_8)
  #define I2C_SDA                 8 
  #define I2C_SCL                 9
#elif defined ESP32DEV_1 || defined (ESP32DEV_2) || defined (ESP32DEV_3) || defined (ESP32DEV_4) || defined (ESP32DEV_5) || defined (ESP32DEV_6)      
  // use default i2c pinoutx2
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
ADS1115_WE          adc = ADS1115_WE(ADS_I2C_ADDRESS);
DS1307              _clock;
// comment either one of this
#ifdef REGULAR_I2C_LCD
  LiquidCrystal_I2C   lcd(0x27,16,2);  // set the LCD address to 0x27 for a 16 chars and 2 line display
#else
  rgb_lcd             lcd;
#endif

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

bool I2C_LCD_AVAILABLE = false;
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
void onRelay2(); // borrow this function to turn on/off i2c LCD LED backlight 
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
  ssidstr = String(ssid);
  if(ssidstr.length() > 16)
    ssidstr.remove(0,16-ssidstr.length());

  Serial.begin(115200);
  #if defined (ESP32DEV_1) || defined (ESP32DEV_2) || defined (ESP32DEV_3) || defined (ESP32DEV_4) || defined (ESP32DEV_5) || defined (ESP32DEV_6)      
    Wire.begin();
  #else
    Wire.begin(I2C_SDA, I2C_SCL);
  #endif
  mac_str = WiFi.macAddress();
  const char* mac_addr = mac_str.c_str();
  Serial.print("MAC (String):");
  Serial.println(mac_str);
  Serial.printf("MAC (const char): %s\n", mac_addr);

  //Increment boot number and print it every reboot
  ++bootCount;
  Serial.println("Boot number: " + String(bootCount));
  //Print the wakeup reason for ESP32
  print_wakeup_reason();

  Serial.print("\n\n **************DEEP SLEEP CONFIG********************* \n");
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  Serial.println("Setup ESP32 to sleep for every " + String(TIME_TO_SLEEP) +
  " Seconds or " + String(SLEEP_HOUR) + " Hours");
  Serial.println("Start Deep Sleep at " + String(setSleepHour) + ":" + String(setSleepMin));
  Serial.print("\n ****************************************************** \n\n");

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

  if(strcmp(mac_addr,"C8:2B:96:B9:A9:58")==0) esp_model = "ESP32DEV-1";
  if(strcmp(mac_addr,"9C:9C:1F:E3:85:3C")==0) esp_model = "ESP32DEV-2";
  if(strcmp(mac_addr,"84:CC:A8:5E:6E:E8")==0) esp_model = "ESP32DEV-3";
  // 7C:9E:BD:07:A8:E4 (Dev-4 with broken usb port)
  if(strcmp(mac_addr,"7C:9E:BD:07:A8:E4")==0) esp_model = "ESP32DEV-4"; // 9C:9C:1F:C5:94:24 who is this?
  if(strcmp(mac_addr,"84:0D:8E:E2:D6:D8")==0) esp_model = "ESP32DEV-5";
  if(strcmp(mac_addr,"9C:9C:1F:C5:94:24")==0) esp_model = "ESP32DEV-6"; // added 19 Jun 2023


  Serial.printf("[setup] %s Found!\n",esp_model);
  Serial.printf("[setup] MAC: %s\n", mac_addr);
  mac_str.remove(2,1); // remove the first : from MAC 

  Serial.println("\nScanning ESP32s2 i2c port...");
  i2c_scan(); // this method discovered to be failed after the latest ESP32 core update 24.02.2023 (Friday)
  Serial.println("\nReinit i2c port");
  #if defined (ESP32DEV_1) || defined (ESP32DEV_2) || defined (ESP32DEV_3) || defined (ESP32DEV_4) || defined (ESP32DEV_5) || defined (ESP32DEV_6)        
    Wire.begin();
  #else
    Wire.begin(I2C_SDA, I2C_SCL);
  #endif  
  Wire.setClock(125000); // discovered 27.02.2023 at Berlian | this is the key line to solve the weird i2c scanning issue 03.03.2023
  
  if(INA219_AVAILABLE){
    initINA219();
  }

  #ifdef REGULAR_I2C_LCD
    lcd.init();                      
    lcd.backlight();
  #else
    lcd.begin(16, 2); // GROVE_LCD_AVAILABLE
  #endif


  if(GROVE_LCD_AVAILABLE || I2C_LCD_AVAILABLE){
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
  pinMode(BLINKER,OUTPUT); // blinker at solar panel

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

  if(GROVE_LCD_AVAILABLE || I2C_LCD_AVAILABLE){
    try_wifi_connect(10); // pre-connection using WiFi.begin() with 10 second default timeout
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Connecting Blynk");
    lcd.setCursor(0,1);
    lcd.write(1); // add arrow before SSID
    lcd.print(ssid);
  }

  if(GROVE_LCD_AVAILABLE || I2C_LCD_AVAILABLE){
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
  restart_ts = get_timestamp(); // second call to reconfirm

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

  if(GROVE_LCD_AVAILABLE || I2C_LCD_AVAILABLE){
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
 

  xTaskCreatePinnedToCore(
      BLYNK_HandlerTask,        /* Task function. */
      "BLYNK",                  /* name of task. */
      10000,                    /* Stack size of task */
      NULL,                     /* parameter of the task */
      1,                        /* priority of the task */
      &Task2,                   /* Task handle to keep track of created task */
      
      CORE_0);                  /* pin task to core 0 */     
  delay(500); 

  xTaskCreatePinnedToCore(
      ESPNOW_HandlerTask,       /* Task function. */
      "ESPNOW",                 /* name of task. */
      10000,                    /* Stack size of task */
      NULL,                     /* parameter of the task */
      1,                        /* priority of the task */
      &Task1,                   /* Task handle to keep track of created task */
      CORE_1);                  /* pin task to core 1 */                  
  delay(500); 


} // end of setup


void loop()
{
  // Blynk.run();
  // timer.run();
}


// callback when data is sent
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}


// callback function that will be executed when data is received
void OnDataRecv(const uint8_t * mac_addr, const uint8_t *incomingData, int len) { 

  // Copies the sender mac address to a string
  char macStr[18];
  Serial.print("Packet received from: ");
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
           mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  // Serial.println(macStr); // UNCOMMENT THIS TO FIGURE OUT THE CLIENT'S MAC 27.08.2022
  if(strcmp(macStr, client1_cchar) == 0)      
  {
    Serial.println("Client ID 1");
  }
  if(strcmp(macStr, client2_cchar) == 0) 
  {
    Serial.println("Client ID 2");
  }
  if(strcmp(macStr, client3_cchar) == 0) 
  {
    Serial.println("Client ESP32C3-3");
  }
  if(strcmp(macStr, client4_cchar) == 0) 
  {
    Serial.println("Client ESP32C3-4");
  }
  // if(strcmp(macStr, client5_cchar) == 0) 
  // {
  //   Serial.println("Client ESP32C3-5");
  // }
  if(strcmp(macStr, client6_cchar) == 0) 
  {
    Serial.println("Client ESP32C3-6");
  }
  if(strcmp(macStr, client7_cchar) == 0) 
  {
    Serial.println("Client ESP32C3-7");
  }
  if(strcmp(macStr, client8_cchar) == 0) 
  {
    Serial.println("Client ESP32C3-8");
  }
  // Serial.println("Unknown Client! Check client's MAC");


  memcpy(&incomingReadings, incomingData, sizeof(incomingReadings));

  // must use double quote for the json label 27.08.2022
  board["d"] = incomingReadings.id; 
  board["#"] = String(incomingReadings.readingId);
  board["v"] = incomingReadings.volt; 
  board["a"] = incomingReadings.amps;  
  board["t"] = incomingReadings.temp; 
  board["h"] = incomingReadings.humi; 
  board["m"] = incomingReadings.mois; 
  board["r"] = incomingReadings.rain; 


  if(incomingReadings.id == "ESP32C3-1"){
    temp1 = incomingReadings.temp;
    humi1 = incomingReadings.humi;
    moist1 = incomingReadings.mois;
    rain1 = incomingReadings.rain;
    volt1 = incomingReadings.volt;
    amps1 = incomingReadings.amps;
    read_id1 = incomingReadings.readingId;
    dt1 = get_timestamp();
    jsonString1 = JSON.stringify(board);
  }
  if(prev_read_id1 != read_id1){
    prev_read_id1 = read_id1;
  }

  if(incomingReadings.id == "ESP32C3-2"){
    temp2 = incomingReadings.temp;
    humi2 = incomingReadings.humi;
    moist2 = incomingReadings.mois;
    rain2 = incomingReadings.rain;
    volt2 = incomingReadings.volt;
    amps2 = incomingReadings.amps;
    read_id2 = incomingReadings.readingId;
    dt2 = get_timestamp();
    jsonString2 = JSON.stringify(board);
  }
  if(prev_read_id2 != read_id2){
    prev_read_id2 = read_id2;
  } 

  if(incomingReadings.id == "ESP32C3-3"){
    temp3 = incomingReadings.temp;
    humi3 = incomingReadings.humi;
    moist3 = incomingReadings.mois;
    rain3 = incomingReadings.rain;
    volt3 = incomingReadings.volt;
    amps3 = incomingReadings.amps;
    read_id3 = incomingReadings.readingId;
    dt3 = get_timestamp();
    jsonString3 = JSON.stringify(board);
  }
  if(prev_read_id3 != read_id3){
    prev_read_id3 = read_id3;
  }

  if(incomingReadings.id == "ESP32C3-4"){
    temp4 = incomingReadings.temp;
    humi4 = incomingReadings.humi;
    moist4 = incomingReadings.mois;
    rain4 = incomingReadings.rain;
    volt4 = incomingReadings.volt;
    amps4 = incomingReadings.amps;
    read_id4 = incomingReadings.readingId;
    dt4 = get_timestamp();
    jsonString4 = JSON.stringify(board);
  }
  if(prev_read_id4 != read_id4){
    prev_read_id4 = read_id4;
  }

  if(incomingReadings.id == "ESP32C3-6"){
    temp6 = incomingReadings.temp;
    humi6 = incomingReadings.humi;
    moist6 = incomingReadings.mois;
    rain6 = incomingReadings.rain;
    volt6 = incomingReadings.volt;
    amps6 = incomingReadings.amps;
    read_id6 = incomingReadings.readingId;
    dt6 = get_timestamp();
    jsonString6 = JSON.stringify(board);
  }
  if(prev_read_id6 != read_id6){
    prev_read_id6 = read_id6;
  }

  if(incomingReadings.id == "ESP32C3-7"){
    temp7 = incomingReadings.temp;
    humi7 = incomingReadings.humi;
    moist7 = incomingReadings.mois;
    rain7 = incomingReadings.rain;
    volt7 = incomingReadings.volt;
    amps7 = incomingReadings.amps;
    read_id7 = incomingReadings.readingId;
    dt7 = get_timestamp();
    jsonString7 = JSON.stringify(board);
  }

  if(prev_read_id7 != read_id7){
    prev_read_id7 = read_id7;
  }

  if(incomingReadings.id == "ESP32C3-8"){
    temp8 = incomingReadings.temp;
    humi8 = incomingReadings.humi;
    moist8 = incomingReadings.mois;
    rain8 = incomingReadings.rain;
    volt8 = incomingReadings.volt;
    amps8 = incomingReadings.amps;
    read_id8 = incomingReadings.readingId;
    dt8 = get_timestamp();
    jsonString8 = JSON.stringify(board);
  }

  if(prev_read_id8 != read_id8){
    prev_read_id8 = read_id8;
  }

  Serial.printf("\nt:%.2f h:%.2f m:%.2f r:%.2f id:%d\n", temp3,humi3,moist3,rain3,read_id3);
  Serial.printf("Volt[3]:%.2f Amps[3]:%.2f \n", volt3, amps3);
  Serial.println(dt3);
  Serial.println(jsonString3);

  Serial.printf("\nt:%.2f h:%.2f m:%.2f r:%.2f id:%d\n", temp4,humi4,moist4,rain4,read_id4);
  Serial.printf("Volt[4]:%.2f Amps[4]:%.2f \n", volt4, amps4);
  Serial.println(dt4);
  Serial.println(jsonString4);

  Serial.printf("\nt:%.2f h:%.2f m:%.2f r:%.2f id:%d\n", temp6,humi6,moist6,rain6,read_id6);
  Serial.println(dt6);
  Serial.printf("Volt[6]:%.2f Amps[6]:%.2f \n", volt6, amps6);
  Serial.println(jsonString6);

  Serial.printf("\nt:%.2f h:%.2f m:%.2f r:%.2f id:%d\n", temp7,humi7,moist7,rain7,read_id7);
  Serial.println(dt7);
  Serial.printf("Volt[7]:%.2f Amps[7]:%.2f \n", volt7, amps7);
  Serial.println(jsonString7);

  Serial.printf("\nt:%.2f h:%.2f m:%.2f r:%.2f id:%d\n", temp8,humi8,moist8,rain8,read_id8);
  Serial.println(dt8);
  Serial.printf("Volt[8]:%.2f Amps[8]:%.2f \n", volt8, amps8);
  Serial.println(jsonString8);

}


void ESPNOW_HandlerTask(void * pvParameters) 
{
  Serial.print("ESPNOW_HandlerTask running on core ");
  Serial.println(xPortGetCoreID());
  // Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  esp_now_initialized = true; // this will allow the BlynkWrite to send out signal via espnow protocol
  
  // Once ESPNow is successfully Init, we will register for recv callback to
  // get recv packer info (this can be used on server and client simultaneously 25.03.2022)
  esp_now_register_recv_cb(OnDataRecv);

  // coming from client-side code start ----------------------------------------------------------
  esp_now_register_send_cb(OnDataSent);

  // Register peer (peer here is going to be client-1) CLIENT ID 1
  esp_now_peer_info_t peerInfo;
  memset(&peerInfo, 0, sizeof(peerInfo)); // https://github.com/espressif/arduino-esp32/issues/6029
  memcpy(peerInfo.peer_addr, client1_mac, 6);
  peerInfo.encrypt = false;
  
  // Add peer CLIENT ID 1      
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add peer client ID 1");
    return;
  }

  // Register peer (peer here is going to be client-2) CLIENT ID 2
  esp_now_peer_info_t peerInfo2;
  memset(&peerInfo2, 0, sizeof(peerInfo2)); // https://github.com/espressif/arduino-esp32/issues/6029
  memcpy(peerInfo2.peer_addr, client2_mac, 6);
  peerInfo2.encrypt = false;
  
  // Add peer CLIENT ID 2       
  if (esp_now_add_peer(&peerInfo2) != ESP_OK){
    Serial.println("Failed to add peer client ID 2");
    return;
  }

    // Register peer (peer here is going to be client-3) CLIENT ID 3
  esp_now_peer_info_t peerInfo3;
  memset(&peerInfo3, 0, sizeof(peerInfo3)); // https://github.com/espressif/arduino-esp32/issues/6029
  memcpy(peerInfo3.peer_addr, client3_mac, 6);
  peerInfo3.encrypt = false;
  
  // Add peer CLIENT ID 3       
  if (esp_now_add_peer(&peerInfo3) != ESP_OK){
    Serial.println("Failed to add peer client ID 3");
    return;
  }

  // Register peer (peer here is going to be client-4) CLIENT ID 4
  esp_now_peer_info_t peerInfo4;
  memset(&peerInfo4, 0, sizeof(peerInfo4)); // https://github.com/espressif/arduino-esp32/issues/6029
  memcpy(peerInfo4.peer_addr, client4_mac, 6);
  peerInfo4.encrypt = false;
  
  // Add peer CLIENT ID 3       
  if (esp_now_add_peer(&peerInfo4) != ESP_OK){
    Serial.println("Failed to add peer client ID 4 (ESP32C3-4)");
    return;
  }

    // Register peer (peer here is going to be client-4) CLIENT ID 4
  esp_now_peer_info_t peerInfo6;
  memset(&peerInfo6, 0, sizeof(peerInfo6)); // https://github.com/espressif/arduino-esp32/issues/6029
  memcpy(peerInfo6.peer_addr, client6_mac, 6);
  peerInfo6.encrypt = false;
  
  // Add peer CLIENT ID 3       
  if (esp_now_add_peer(&peerInfo6) != ESP_OK){
    Serial.println("Failed to add peer client ID 6 (ESP32C3-6)");
    return;
  }

  // Register peer (peer here is going to be client-8) CLIENT ID 8
  esp_now_peer_info_t peerInfo8;
  memset(&peerInfo8, 0, sizeof(peerInfo8)); // https://github.com/espressif/arduino-esp32/issues/6029
  memcpy(peerInfo8.peer_addr, client8_mac, 6);
  peerInfo8.encrypt = false;
  
  // Add peer CLIENT ID 3       
  if (esp_now_add_peer(&peerInfo8) != ESP_OK){
    Serial.println("Failed to add peer client ID 8 (ESP32C3-8)");
    return;
  }

  // esp_now_peer_num_t 
  // coming from client-side code end ----------------------------------------------------------

  // leave this empty to reserve more CPU resources for other intensive tasks    
  for(;;){} // empty forever loop (no need to do anything inside here otherwise it will cost some CPU time)
} // end of FreeRTOS handler
void BLYNK_HandlerTask(void * pvParameters) 
{

  timer.setInterval(1000L, BLYNK_TASK);
  timer.setInterval(1000L, get_weather);

  Serial.print("BLYNK_HandlerTask running on core ");
  Serial.println(xPortGetCoreID());
  for(;;){    
    Blynk.run();
    timer.run();
  }
} // end of FreeRTOS handler


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
                lcd.print(ssid);
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
    int strlen = ssidstr.length()+1;
    char ssidchar[strlen];
    ssidstr.toCharArray(ssidchar,strlen); 
   
    if(GROVE_LCD_AVAILABLE || I2C_LCD_AVAILABLE){
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("-WiFi Connected-");
      lcd.setCursor(0,1);
      lcd.print(ssid);
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
  int currentHour = timeClient.getHours();
  int currentMin = timeClient.getMinutes();
  int currentsec = timeClient.getSeconds();
  int splitT      = formattedDate.indexOf("T");
  dayStamp        = formattedDate.substring(5, splitT);
  String dateTime = timeClient.getFormattedTime() + " " + dayStamp;

  bool pushAlready = false;

  if(deep_sleep_activated){
    pushAlready = false; // reset this logic
    etaHour = setSleepHour - currentHour;
    etaMin = 60 - currentMin;
    etaSec = 60 - currentsec;

    etaDS = String(etaHour)+":"+String(etaMin)+":"+String(etaSec);  
    Blynk.virtualWrite(V3, "DS starts in T-"+ etaDS);
    if((currentHour >= setSleepHour && currentMin >= setSleepMin) || (currentHour >= 0 && currentHour < 7)){
      Serial.println("Going to sleep now. Pushing Deep Sleep Timestamp of " + dateTime);
      int onlineHour = currentHour + SLEEP_HOUR - 24; // calc reconnection time
      Serial.println("Will be back online at " + String(onlineHour) + ":" + String(currentMin));
      Blynk.virtualWrite(V3, "DS started at "+ dateTime);
      Serial.flush(); 
      lcd.clear();
      lcd.setCursor(0, 0); // row 0, column 0
      lcd.print("-Back Online at:");
      lcd.setCursor(0, 1); // row 0, column 0
      String onHour, onMin;
      if(onlineHour < 10)
        onHour = "0"+String(onlineHour);
      if(currentMin < 10)
        onMin = "0"+String(currentMin);
      lcd.print(onHour+ ":" + onMin + " V:" + String(busvoltage));
      delay(1000);
      #ifdef REGULAR_I2C_LCD
        lcd.noBacklight();
      #else
        offRelay2();
      #endif
      leds.setColorRGB(i, 0, 0, 0); // turn off RGB
      delay(1000);
      esp_deep_sleep_start(); // go to sleep and wake up when timer timeout
    }
  }else{
    if(!pushAlready){
      Blynk.virtualWrite(V3, restart_ts); 
      pushAlready = true;
    }
  }
  return dateTime;
}


void off_onboard_led(){
  digitalWrite(BLINKER,0);
   #ifdef ESP32C3
      digitalWrite(ONBOARD_LED, LOW); // LOW    = OFF LED on ESP32 Dev and C3
   #else
      digitalWrite(ONBOARD_LED, HIGH); // HIGH  = OFF LED on ESP32S2
   #endif  
}

void on_onboard_led(){
  digitalWrite(BLINKER,1);
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
  #ifdef REGULAR_I2C_LCD
    lcd.backlight();
  #endif
}
void offRelay2(){
  digitalWrite(IN2,0);
  #ifdef REGULAR_I2C_LCD
    lcd.noBacklight();
  #endif
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
  if(GROVE_LCD_AVAILABLE || I2C_LCD_AVAILABLE){
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("Reset WiFi Retry!");
      lcd.setCursor(0,1);
      lcd.print("Retry Count Reset");
    }
    Serial.println("@@@@@@@@@@@@@@@@@@ CLEARING WiFi Retry Counter @@@@@@@@@@@@@@@@@@ ");
    write16bitIntoEEPROM(wifiConnRetryAddress,0);
}

void clear_disconnCounter(){
    if(GROVE_LCD_AVAILABLE || I2C_LCD_AVAILABLE){
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
    if(GROVE_LCD_AVAILABLE || I2C_LCD_AVAILABLE){
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
      if(address == 0x27) I2C_LCD_AVAILABLE = true;
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
      /*
      C3-3 to AASAS M10 (TBS)
      C3-4 to AASAS M03 (MKE)
      C3-8 to AASAS M10 (TBS)
      */
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
    // Blynk.virtualWrite(V12, jsonWeather); // disabled temporarily
    Blynk.virtualWrite(V12, "C3-8 = " + jsonString8); // this should be C8 at TBS (need to commit DEV_5 as well for the change)
    Blynk.virtualWrite(V15, "C3-6 = " + jsonString6); // taking over disconnected ts

    Blynk.virtualWrite(V34, get_ambient_temp());

    if(GROVE_LCD_AVAILABLE || I2C_LCD_AVAILABLE)
      lcd.clear();

    if(displayESPNOW){
        if(GROVE_LCD_AVAILABLE || I2C_LCD_AVAILABLE){
          #if defined LOCATION_MKE2_MaxisONE || defined (LOCATION_MKE2_UiTM_WiFi_IoT)
            lcd.setCursor(0,0); // MKE2 
            lcd.print("C3-4 #"+String(read_id4)+" V:" + String(volt4)); // print connected SSID
            lcd.setCursor(0,1);
            lcd.print("C3-7 #"+String(read_id7)+" V:" + String(volt7)); // print connected SSID
          #else
            lcd.setCursor(0,0); // TBS
            lcd.print("C3-6 #"+String(read_id6)+" V:" + String(volt6)); // print connected SSID
            lcd.setCursor(0,1); // MKE2
            lcd.print("C3-8 #"+String(read_id8)+" V:" + String(volt8)); // print connected SSID
          #endif
        }
    }
    else if(deep_sleep_activated){
      if(GROVE_LCD_AVAILABLE || I2C_LCD_AVAILABLE){
        lcd.setCursor(0,0); // row 0, column 0
        lcd.print("DS ETA: "+ etaDS); // print connected SSID
        lcd.setCursor(0,1); // row 0, column 0
        lcd.print(String(busvoltage)+"V I:"+String(current_mA)+"mA");
      }
    }
    else{
       if(millis() % 2 == 0){
          if(GROVE_LCD_AVAILABLE || I2C_LCD_AVAILABLE){
            lcd.setCursor(0,0); // row 0, column 0
            lcd.print("----"+ String(BLYNK_DEVICE_NAME)+"---"); // ----AASAS ONE---
            lcd.setCursor(0,1);
            lcd.print(dateTime);
          }
        }
        else if(millis() % 4 == 0){
          if(GROVE_LCD_AVAILABLE || I2C_LCD_AVAILABLE){
            lcd.setCursor(0,0); // row 0, column 0
            lcd.print(esp_model); // print connected SSID
            lcd.setCursor(0,1); // row 0, column 0
            lcd.print(mac_str); // print connected SSID
          }
        }
        else if(millis() % 6 == 0){
          if(GROVE_LCD_AVAILABLE || I2C_LCD_AVAILABLE){
            lcd.setCursor(0,0); // row 0, column 0
            lcd.print("RSSI: "+String(RSSI_dBm)+" dBm"); // print connected SSID
            lcd.setCursor(0,1); // row 0, column 0
            lcd.print(get_rssi_state(RSSI_dBm)); // print connected SSID
          }
        }
        else if(millis() % 8 == 0){
          if(GROVE_LCD_AVAILABLE || I2C_LCD_AVAILABLE){
            lcd.setCursor(0,0); // row 0, column 0
            lcd.print("DS ETA: "+ etaDS); // print connected SSID
            lcd.setCursor(0,1); // row 0, column 0
            lcd.print(String(busvoltage)+"V I:"+String(current_mA)+"mA");
          }
        }
        else{
          if(GROVE_LCD_AVAILABLE || I2C_LCD_AVAILABLE){
            display_uptime_top_row();
            lcd.setCursor(0,1); // row 0, column 0
            lcd.print(String(busvoltage)+"V I:"+String(current_mA)+"mA");
          }
        }    
    } // close else for espnow display
    rgb_LED_Off();      
}

BLYNK_CONNECTED() {
  blueOn();
  if(GROVE_LCD_AVAILABLE || I2C_LCD_AVAILABLE){
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
    if(GROVE_LCD_AVAILABLE || I2C_LCD_AVAILABLE){
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
  if(GROVE_LCD_AVAILABLE || I2C_LCD_AVAILABLE){
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
  if(GROVE_LCD_AVAILABLE || I2C_LCD_AVAILABLE){
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

// borrow V25 for LED backlight on/off 
BLYNK_WRITE(V25){
  int val = param.asInt();
  if(val) onRelay2();
  else    offRelay2();
}
// borrow V26 for esp_deep_sleep function
BLYNK_WRITE(V26){
  int val = param.asInt();
  if(val) onRelay3();
  else    offRelay3();

  if(val)       deep_sleep_activated = true;
  else          deep_sleep_activated = false;
}

BLYNK_WRITE(V27){
  int pinValue = param.asInt();
  if(pinValue) clear_disconnCounter();
}

BLYNK_WRITE(V28){
  int pinValue = param.asInt();
  if(pinValue) clear_wifiRetryCounter();
  if(pinValue) displayESPNOW = true;
  else         displayESPNOW = false;
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
  waterLvlPercent = map(distance_cm,90,40,0,100);  

  // We print the distance on the serial port
  // Serial.print("Distance (cm): ");
  // Serial.println(distance_cm);
  return distance_cm;
}


void print_wakeup_reason(){
  esp_sleep_wakeup_cause_t wakeup_reason;

  wakeup_reason = esp_sleep_get_wakeup_cause();

  switch(wakeup_reason)
  {
    case ESP_SLEEP_WAKEUP_EXT0 : Serial.println("Wakeup caused by external signal using RTC_IO"); break;
    case ESP_SLEEP_WAKEUP_EXT1 : Serial.println("Wakeup caused by external signal using RTC_CNTL"); break;
    case ESP_SLEEP_WAKEUP_TIMER : Serial.println("Wakeup caused by timer"); break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD : Serial.println("Wakeup caused by touchpad"); break;
    case ESP_SLEEP_WAKEUP_ULP : Serial.println("Wakeup caused by ULP program"); break;
    default : Serial.printf("Wakeup was not caused by deep sleep: %d\n",wakeup_reason); break;
  }
}

