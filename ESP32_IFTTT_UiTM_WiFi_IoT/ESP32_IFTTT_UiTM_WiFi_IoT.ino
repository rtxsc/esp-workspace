/*
Testing UiTM WiFi IoT 9/12/2022 Friday
Works with ESP32S2
Edited 28 Dec 2022
Subscribed to Blynk Plus RM30.90/month on Monday 13 Feb 2023
Connected and Disconnected logic furnished 15 Feb 2023

solving MD5 flash issue Sunday 18 June 2023 at TBS
python3 -m esptool --chip esp32 write_flash_status --non-volatile 0
python3 -m esptool --chip esp32 erase_flash 

need to limit INA219 readout from ESPNOW to 2 decimal places 28 June 2023
change the struct on both server and client
*/
// #define ESP32S2_1  // M01
// #define ESP32S2_2  // M02
// #define ESP32S2_3  // M03
// #define ESP32S2_4  // M04
#define ESP32S2_5  // M05
// #define ESP32S2_6  // M06
// #define ESP32DEV_1 // M07 control water valve & pump
// #define ESP32DEV_2 // M08 @ water level sensing
// #define ESP32DEV_3
// #define ESP32DEV_4 // DEV_4 NEW TBS front gate (perfect DEV)
// #define ESP32DEV_5 // M10
// #define ESP32DEV_6 // TBS front gate (perfect DEV)
// #define ESP32DEV_0 // DEV_0 NEW TBS front gate (perfect DEV)
// #define ESP32C3_4

// #define REGULAR_I2C_LCD // comment if using GROVE_LCD
#define LOCATION_MKE2_UiTM_WiFi_IoT // comment this line for TBS deployment
// #define LOCATION_MKE2_MaxisONE // comment this line for MaxisONE Fibre 2.4G_EXT
// #define Maxis_Postpaid_128

/*
V1 dateTime
V2 WiFi.localIP().toString()
V3 restart_ts & deep sleep info
V4 RSSI_dBm
V5 restartCounter
V6 busvoltage
V7 current_mA
V8 ssid
V9 get_rssi_state(RSSI_dBm)
V10 tempC
V11 humid
V12 json weather & jsonstring espnow
V13 not use
V14 not use
V15 disconn_ts_str & jsonstring espnow
V16 disconnection_count
V17 blynk_on_connected_ts
V18 ads_readout & water level
V19 wifiRetryCount
V20 BLYNK_WRITE R
V21 BLYNK_WRITE G
V22 BLYNK_WRITE B
V23 BLYNK_WRITE Clear RC
V24 BLYNK_WRITE Relay 1
V25 BLYNK_WRITE Relay 2
V26 BLYNK_WRITE Relay 3
V27 BLYNK_WRITE clear_disconnCounter
V28 BLYNK_WRITE clearWiFiRetry + espnow on/off
V29 default_hostname

V30 not use (going to be get_ambient_temp) GAUGE
V31 not use waterlevel GAUGE
V32 not use espnow c3-4 volt GAUGE
V33 not use espnow c3-7 volt GAUGE
V34 get_ambient_temp ----> espnow c3-4 amp GAUGE
V35 not use espnow c3-7 amp GAUGE
V36 not use espnow c3-4 moisture GAUGE
V37 not use espnow c3-7 moisture GAUGE
V38 not use espnow c3-4 soil temp GAUGE
V39 not use espnow c3-7 soil temp GAUGE

V40 Button LED Backlight
V41 Button Deep Sleep
V42 Button Display espnow
*/

bool deep_sleep_activated = false;
// deep sleep config
#define uS_TO_S_FACTOR 1000000ULL  /* Conversion factor for micro seconds to seconds */
#define SLEEP_HOUR     13
#define TIME_TO_SLEEP  SLEEP_HOUR*3600       /* Time ESP32 will go to sleep (in seconds) 50400 */
#define WAKEUP_HOUR    6
int setSleepHour = 18;
int setSleepMin = 0;
uint8_t etaHour = 0;
uint8_t etaMin = 0;
uint8_t etaSec = 0;
String etaDS = "HH:MM:SS";
RTC_DATA_ATTR int bootCount = 0;

int elapseEntry = 0;
int elapseExit = 0;
int elapseSec = 0;
int e_h, e_m, e_s;
String e_HourStr, e_MinStr, e_SecStr;
String e_elapse;

/* Comment this out to disable prints and save space */
#define BLYNK_PRINT Serial
#define BLYNK_TEMPLATE_ID   "TMPLjsD8y_SC" // template ID for M04 and M05 then subcribing to Plus RM30.90 13Feb2023
#define BLYNK_TEMPLATE_NAME "AASAS UiTM WiFi IoT"

#if defined ESP32S2_1
  #define BLYNK_DEVICE_NAME "AASAS M01"
  #define BLYNK_AUTH_TOKEN "J3DXwnJNxoCIUI3TF7ULHmCKdDg27FV4"
#elif defined ESP32S2_2
  #define BLYNK_DEVICE_NAME "AASAS M02"
  #define BLYNK_AUTH_TOKEN "AgRl8rHXRFkz4KnUvFWfQtlUCGZ6g8ug"
#elif defined ESP32S2_3
  #define BLYNK_DEVICE_NAME "AASAS M03"
  #define BLYNK_AUTH_TOKEN "vdnuDWozgCa3oP3_xXnekRTLmlo2mjuk"
#elif defined ESP32S2_4 || defined ESP32DEV_4 || defined ESP32DEV_6
  #define BLYNK_DEVICE_NAME "AASAS M04"
  #define BLYNK_AUTH_TOKEN "gee5lkJxSCmQrqplsAiH-uVPuNkF-B3G"
#elif defined ESP32S2_5 || defined ESP32DEV_2
  #define BLYNK_DEVICE_NAME "IFTTT M05"
  #define BLYNK_AUTH_TOKEN "Rq548So3QmWpZIJyAt59TVmW8W4GGlUd"
#elif defined ESP32S2_6
  #define BLYNK_DEVICE_NAME "AASAS M06"
  #define BLYNK_AUTH_TOKEN "qtER7nxNJH1QioLqmH_rE688VdJ5i0L0"
#elif defined ESP32DEV_1
  #define BLYNK_DEVICE_NAME "AASAS M07"
  #define BLYNK_AUTH_TOKEN "K-NDkXmOkZNC3TIKZo6EOrqdQQ8-Mr_f"
#elif defined ESP32DEV_2XXX
  #define BLYNK_DEVICE_NAME "AASAS M08"
  #define BLYNK_AUTH_TOKEN "VoVZgVSmyKThZyh8KSS9oNq7gEcPLk0b"
#elif defined ESP32DEV_5
  #define BLYNK_DEVICE_NAME "AASAS M10"
  #define BLYNK_AUTH_TOKEN "Wo6dEX9FFGRjR-fbRShY-FxM9uYAYdqp"
#elif defined ESP32DEV_0// ESP32DEV_4 NEW
  #define BLYNK_DEVICE_NAME "AASAS M09" 
  #define BLYNK_AUTH_TOKEN "ZELuVGWY16O3KPW8mLkgkdISj2ohVgP7"
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

#include "WiFiClientSecure.h"
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"

#define AIO_SERVER      "io.adafruit.com"
// Using port 8883 for MQTTS
#define AIO_SERVERPORT  8883                  
#define AIO_USERNAME    "clumzyazid"
// #define AIO_KEY         "aio_VGkW373G4psoR4xzLlG4WvOsmvue" // disabled 15 Nov after GitHub leaked
#define AIO_KEY         "aio_kDjS914Yqk7rDbXUt5s1f2gWjngx" // new key obtained 16 Nov 2023

String entryStatus = "None";

WiFiClientSecure client;

Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);    

// io.adafruit.com root CA
const char* adafruitio_root_ca = \
      "-----BEGIN CERTIFICATE-----\n"
      "MIIEjTCCA3WgAwIBAgIQDQd4KhM/xvmlcpbhMf/ReTANBgkqhkiG9w0BAQsFADBh\n"
      "MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3\n"
      "d3cuZGlnaWNlcnQuY29tMSAwHgYDVQQDExdEaWdpQ2VydCBHbG9iYWwgUm9vdCBH\n"
      "MjAeFw0xNzExMDIxMjIzMzdaFw0yNzExMDIxMjIzMzdaMGAxCzAJBgNVBAYTAlVT\n"
      "MRUwEwYDVQQKEwxEaWdpQ2VydCBJbmMxGTAXBgNVBAsTEHd3dy5kaWdpY2VydC5j\n"
      "b20xHzAdBgNVBAMTFkdlb1RydXN0IFRMUyBSU0EgQ0EgRzEwggEiMA0GCSqGSIb3\n"
      "DQEBAQUAA4IBDwAwggEKAoIBAQC+F+jsvikKy/65LWEx/TMkCDIuWegh1Ngwvm4Q\n"
      "yISgP7oU5d79eoySG3vOhC3w/3jEMuipoH1fBtp7m0tTpsYbAhch4XA7rfuD6whU\n"
      "gajeErLVxoiWMPkC/DnUvbgi74BJmdBiuGHQSd7LwsuXpTEGG9fYXcbTVN5SATYq\n"
      "DfbexbYxTMwVJWoVb6lrBEgM3gBBqiiAiy800xu1Nq07JdCIQkBsNpFtZbIZhsDS\n"
      "fzlGWP4wEmBQ3O67c+ZXkFr2DcrXBEtHam80Gp2SNhou2U5U7UesDL/xgLK6/0d7\n"
      "6TnEVMSUVJkZ8VeZr+IUIlvoLrtjLbqugb0T3OYXW+CQU0kBAgMBAAGjggFAMIIB\n"
      "PDAdBgNVHQ4EFgQUlE/UXYvkpOKmgP792PkA76O+AlcwHwYDVR0jBBgwFoAUTiJU\n"
      "IBiV5uNu5g/6+rkS7QYXjzkwDgYDVR0PAQH/BAQDAgGGMB0GA1UdJQQWMBQGCCsG\n"
      "AQUFBwMBBggrBgEFBQcDAjASBgNVHRMBAf8ECDAGAQH/AgEAMDQGCCsGAQUFBwEB\n"
      "BCgwJjAkBggrBgEFBQcwAYYYaHR0cDovL29jc3AuZGlnaWNlcnQuY29tMEIGA1Ud\n"
      "HwQ7MDkwN6A1oDOGMWh0dHA6Ly9jcmwzLmRpZ2ljZXJ0LmNvbS9EaWdpQ2VydEds\n"
      "b2JhbFJvb3RHMi5jcmwwPQYDVR0gBDYwNDAyBgRVHSAAMCowKAYIKwYBBQUHAgEW\n"
      "HGh0dHBzOi8vd3d3LmRpZ2ljZXJ0LmNvbS9DUFMwDQYJKoZIhvcNAQELBQADggEB\n"
      "AIIcBDqC6cWpyGUSXAjjAcYwsK4iiGF7KweG97i1RJz1kwZhRoo6orU1JtBYnjzB\n"
      "c4+/sXmnHJk3mlPyL1xuIAt9sMeC7+vreRIF5wFBC0MCN5sbHwhNN1JzKbifNeP5\n"
      "ozpZdQFmkCo+neBiKR6HqIA+LMTMCMMuv2khGGuPHmtDze4GmEGZtYLyF8EQpa5Y\n"
      "jPuV6k2Cr/N3XxFpT3hRpt/3usU/Zb9wfKPtWpoznZ4/44c1p9rzFcZYrWkj3A+7\n"
      "TNBJE0GmP2fhXhP1D/XVfIW/h0yCJGEiV9Glm/uGOa3DXHlmbAcxSyCRraG+ZBkA\n"
      "7h4SeM6Y8l/7MBRpPCz6l8Y=\n"
      "-----END CERTIFICATE-----\n";

Adafruit_MQTT_Subscribe ESP32S2_RGB_SW = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/ResetFeed");
Adafruit_MQTT_Subscribe locationTracker = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/LocationAwareness"); // added 8 Nov 2023 Wednesday

// Setup a feed called 'test' for publishing.
// Notice MQTT paths for AIO follow the form: <username>/feeds/<feedname>
Adafruit_MQTT_Publish pubTracker = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/TOGGLE");

void MQTT_connect();


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

//48:27:E2:5D:C5:1C (S2m1)
uint8_t client9_mac[] = {0x48, 0x27, 0xE2, 0x5D, 0xC5, 0x1C}; // changed 10.june.2023 after SKJMT
String client9_mac_string = "48:27:e2:5d:c5:1c"; // added S2m1 26 June 2023 after condemning ESP32C3-3
const char* client9_cchar = client9_mac_string.c_str(); // changed 10.june.2023 after SKJMT


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

#if defined (ESP32DEV_0) || defined (ESP32DEV_1) || defined (ESP32DEV_2) || defined (ESP32DEV_3) || defined (ESP32DEV_4) || defined (ESP32DEV_5) || defined (ESP32DEV_6)     
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
#if defined (ESP32DEV_0) || defined (ESP32DEV_1) || defined (ESP32DEV_2) || defined (ESP32DEV_3) || defined (ESP32DEV_4) || defined (ESP32DEV_5) || defined (ESP32DEV_6)       
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
  char pass[] = "respironics"; 
#elif defined Maxis_Postpaid_128
  char ssid[] = "Maxis Postpaid 128";
  char pass[] = "respironics"; 
#else
  char ssid[] = "MaxisONE Fibre 2.4G_EXT"; 
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
#elif defined ESP32DEV_0 || defined ESP32DEV_1 || defined (ESP32DEV_2) || defined (ESP32DEV_3) || defined (ESP32DEV_4) || defined (ESP32DEV_5) || defined (ESP32DEV_6)      
  // use default i2c pinoutx2
#else
  #define I2C_SDA                 41 
  #define I2C_SCL                 40
#endif

#define wifiConnRetryAddress    0x0C // DEC 12 & 13 : 2 bytes 
#define restartCounterAddress   0x0F // DEC 15 & 16 : 2 bytes F 10 11
#define disconnCounterAddress   0x12 // DEC 18 & 19 bytes
#define disconnectTS__Address   0x14 // DEC 20 - 34 (HEX 14 - 22) and beyond 
// ******************************** IFTTT ENTRY STATUS DECLARATION STARTS ********************************
#define entryStatusAddress      0x23 // HEX 0x23 (DEC 35)
#define entryTimestampAddress   0x24 // HEX 0x24 - 32 (DEC 36 - 50) ... 24 25 26 27 28 29 2A 2B 2C 2D 2E 2F 30 31 32
#define elapseAdd               0x33 // HEX 0x33 - 41  (DEC 51 - 65) ... 33 34 35 36 37 38 39 3A 3B 3C 3D 3E 3F 40 41 .. 41 – 33 = E ... 65 – 51 = 14
uint8_t entryState = 0; // 0 exited || 1 entered || 2 keluar || 3 masuk || 4 unknown
String ets = "None";
// ******************************** IFTTT ENTRY STATUS DECLARATION ENDS ********************************
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

float shuntvoltage = -1.0;
float busvoltage = -1.0;
float current_mA = -1.0;

bool I2C_LCD_AVAILABLE = false;
bool GROVE_LCD_AVAILABLE = false; // true for debugging purpose 27.02.2023 | change to false 03.03.2023
bool INA219_AVAILABLE = false;
bool ADS1115_AVAILABLE = false;

#define EEPROM_SIZE             100
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
BLYNK_WRITE(V27);

double round_2dp(double x); // prototype for func to reduce floating precision
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

void onRelay1(); // 12V DC solenoid water valve
void onRelay2(); // borrow this function to turn on/off i2c LCD LED backlight 
void onRelay3(); // 12V DC water pump
void offRelay1();
void offRelay2();
void offRelay3();
String get_timestamp();

bool openNetwork = false;

String openWeatherMapApiKey = "dbd7235bcc77e5896c73e975d013debe";
String city = "Kuching";
String countryCode = "MY";
unsigned long lastTime = 0;
unsigned long interval = 60000;

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
  #if defined (ESP32DEV_0) || defined (ESP32DEV_1) || defined (ESP32DEV_2) || defined (ESP32DEV_3) || defined (ESP32DEV_4) || defined (ESP32DEV_5) || defined (ESP32DEV_6)      
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

  if(strcmp(mac_addr,"7C:9E:BD:07:3A:98")==0) esp_model = "ESP32DEV-0_NEW";
  if(strcmp(mac_addr,"C8:2B:96:B9:A9:58")==0) esp_model = "ESP32DEV-1";
  if(strcmp(mac_addr,"9C:9C:1F:E3:85:3C")==0) esp_model = "ESP32DEV-2";
  if(strcmp(mac_addr,"84:CC:A8:5E:6E:E8")==0) esp_model = "ESP32DEV-3";
  if(strcmp(mac_addr,"4C:11:AE:D7:9B:E0")==0) esp_model = "ESP32DEV-4_NEW"; // NEW
  if(strcmp(mac_addr,"84:0D:8E:E2:D6:D8")==0) esp_model = "ESP32DEV-5";
  if(strcmp(mac_addr,"9C:9C:1F:C5:94:24")==0) esp_model = "ESP32DEV-6"; // added 19 Jun 2023
  
  if(strcmp(mac_addr,"48:27:E2:5D:C5:1C")==0) esp_model = "ESP32S2m1"; // added 19 Jun 2023

  Serial.printf("[setup] %s Found!\n",esp_model);
  Serial.printf("[setup] MAC: %s\n", mac_addr);
  mac_str.remove(2,1); // remove the first : from MAC 

  Serial.println("\nScanning ESP32s2 i2c port...");
  i2c_scan(); // this method discovered to be failed after the latest ESP32 core update 24.02.2023 (Friday)
  Serial.println("\nReinit i2c port");
  #if defined (ESP32DEV_0) || defined (ESP32DEV_1) || defined (ESP32DEV_2) || defined (ESP32DEV_3) || defined (ESP32DEV_4) || defined (ESP32DEV_5) || defined (ESP32DEV_6)        
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
    delay(1000);
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
  // clear_eeprom();
  // write16bitIntoEEPROM(wifiConnRetryAddress, 0); // comment this after debugging || the first upload

  // restartCounter = EEPROM.read(restartCounterAddress);
  // 0 exited || 1 entered || 2 keluar || 3 masuk || 4 unknown
  entryState = EEPROM.read(entryStatusAddress);
  ets = readStringFromEEPROM(entryTimestampAddress);
  restartCounter = read16bitFromEEPROM(restartCounterAddress);
  disconnection_count = read16bitFromEEPROM(disconnCounterAddress);
  wifiRetryCount = read16bitFromEEPROM(wifiConnRetryAddress);
  elapseSec = read16bitFromEEPROM(elapseAdd);
  if(wifiRetryCount > 65535) wifiRetryCount = 0;
  check_restart_count();
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("-Read IFTTT NFO-");
  lcd.setCursor(0,1);
  lcd.print("ETS n EntryState");
  delay(1000);

  ets = readStringFromEEPROM(entryTimestampAddress);
  entryState = EEPROM.read(entryStatusAddress);

  if(entryState == 0){
    entryStatus = "EXITED[M]";
    elapseExit = elapseSec;
    redOn();
  }
  else if(entryState == 1){
    entryStatus = "ENTERED[M]";
    elapseEntry = elapseSec;
    blueOn();
  }
  else if(entryState == 2){
    entryStatus = "KELUAR[M]";
    elapseExit = elapseSec;
    redOn();
  }
  else if(entryState == 3){
    entryStatus = "MASUK[M]";
    elapseEntry = elapseSec;
    greenOn();
  }
  else if(entryState == 99)
    entryStatus = "WEIRD[M]";
  else
    entryStatus = "ets#:" + String(entryState);

  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print(">>Read completed!");
  lcd.setCursor(0,1);
  lcd.print("Result...........");
  delay(1000);
  
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print(ets);
  lcd.setCursor(0,1);
  lcd.print(entryStatus);
  delay(2000);

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

  ssidstr = String(ssid);
  if(ssidstr.length() > 16)
    ssidstr.remove(0,ssidstr.length()-16); // logic corrected at MKE2 18:58 Wed 21 June

  if(GROVE_LCD_AVAILABLE || I2C_LCD_AVAILABLE){
    try_wifi_connect(10); // pre-connection using WiFi.begin() with 10 second default timeout
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Connecting WiFi");
    lcd.setCursor(0,1);
    lcd.write(1); // add arrow before SSID
    lcd.print(ssid);
  }

  if(GROVE_LCD_AVAILABLE || I2C_LCD_AVAILABLE){
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Init TimeClient!");
    lcd.setCursor(0,1);
    lcd.print("-Get restart TS!");
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

  // disable this for IFTTT code
  // Blynk.begin(auth, ssid, pass); // connectWiFi in Blynk is active. meaning connecting twice

  if(GROVE_LCD_AVAILABLE || I2C_LCD_AVAILABLE){
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("IFTTT Connected!");
    lcd.setCursor(0,1);
    lcd.print("-"+String(BLYNK_DEVICE_NAME)+" INIT-");
  }
  on_onboard_led();
  pixels.begin(); 
  loopRGB();
  rgbOff();
 

  // xTaskCreatePinnedToCore(
  //     BLYNK_HandlerTask,        /* Task function. */
  //     "BLYNK",                  /* name of task. */
  //     10000,                    /* Stack size of task */
  //     NULL,                     /* parameter of the task */
  //     1,                        /* priority of the task */
  //     &Task2,                   /* Task handle to keep track of created task */
      
  //     CORE_0);                  /* pin task to core 0 */     
  // delay(500); 

  // xTaskCreatePinnedToCore(
  //     ESPNOW_HandlerTask,       /* Task function. */
  //     "ESPNOW",                 /* name of task. */
  //     10000,                    /* Stack size of task */
  //     NULL,                     /* parameter of the task */
  //     1,                        /* priority of the task */
  //     &Task1,                   /* Task handle to keep track of created task */
  //     CORE_1);                  /* pin task to core 1 */                  
  // delay(500); 

  mqtt.subscribe(&ESP32S2_RGB_SW);
  mqtt.subscribe(&locationTracker); // added 8 Nov 2023 Wednesday

  // Set Adafruit IO's root CA
  client.setCACert(adafruitio_root_ca);

} // end of setup


// void loop()
// {
//   // Blynk.run();
//   // timer.run();
// }

uint32_t count=0;
uint8_t x = 0;
const char* pubStr = "ESP recv ";

void loop() {
  // Ensure the connection to the MQTT server is alive (this will make the first
  // connection and automatically reconnect when disconnected).  See the MQTT_connect
  // function definition further below.
  MQTT_connect();

  // // Now we can publish stuff!
  // Serial.print(F("\nSending val "));
  // Serial.print(x);
  // Serial.print(F(" to pubTracker feed..."));
  // if (! pubTracker.publish(x++)) {
  //   Serial.println(F("Failed"));
  // } else {
  //   Serial.println(F("OK!"));
  // }
  // delay(5000);

  Adafruit_MQTT_Subscribe *subscription;
  while ((subscription = mqtt.readSubscription(1000))) { // timeout here is also acting like delay interval for the entire code
    if (subscription == &ESP32S2_RGB_SW) {
      Serial.print(F("\n[ResetFeed] ESP32S2_RGB_SW Got: "));
      Serial.println((char *)ESP32S2_RGB_SW.lastread);
      if (!strcmp((char*) ESP32S2_RGB_SW.lastread, "1")){
        rgbOff();
        if (! pubTracker.publish("Restarting ESP32")) {
          Serial.println(F("Failed"));
        } else {
          Serial.println(F("OK! Restart now"));
          lcd.setCursor(0, 0); // row 1, column 0
          lcd.print("Request Restart"); // default_hostname
          lcd.setCursor(0, 1); // row 1, column 0
          lcd.print("Bye-bye.......");
          disconnected_ts = get_timestamp();
          writeStringToEEPROM(disconnectTS__Address, disconnected_ts);
          delay(1000);
          ESP.restart();
        }
      }

      // if (!strcmp((char*) ESP32S2_RGB_SW.lastread, "ON") || !strcmp((char*) ESP32S2_RGB_SW.lastread, "TOGGLE"))
      // {
      //   count++;
      //   Serial.print("count=");
      //   Serial.println(count);
      // }
      // if(count % 2 == 0)
      //   redOn();
      // else
      //   rgbOff();
  
    } // end of ESP32S2_RGB_SW

    // locationTracker added 8 Nov 2023 Wednesday
    // 0 exited || 1 entered || 2 keluar || 3 masuk || 4 unknown
    if(subscription == &locationTracker){
      Serial.print(F("\nGPS Tracker Got: "));
      // Serial.println((char *)locationTracker.lastread);     
      entryStatus = (char *)locationTracker.lastread; // added 17 Nov 2023 after 99 weirdness in gps detection

      if(!strcmp(entryStatus.c_str(), "masuk")){
        entryStatus = "MASUK";
        entryState = 3;
        greenOn();
        // if (! pubTracker.publish("Sent ESP32 received masuk (switch)")) {
        //   Serial.println(F("Failed"));
        // } else {
        //   Serial.println(F("OK! Sent ESP32 received masuk (switch)"));
        // }
      }
      else if(!strcmp(entryStatus.c_str(), "keluar")){
        entryStatus = "KELUAR";
        entryState = 2;
        redOn();
        // if (! pubTracker.publish("Sent ESP32 received keluar (switch)")) {
        //   Serial.println(F("Failed"));
        // } else {
        //   Serial.println(F("OK! Sent ESP32 received keluar (switch)"));
        // }
      }
      else{
        // entryStatus = (char *)locationTracker.lastread; // added 19 Nov 2023
         if(!strcmp(entryStatus.c_str(), "exited")) {
            entryStatus = "GPSexited";
            entryState = 0;
            redOn(); 
          }
          if(!strcmp(entryStatus.c_str(), "entered")) {
            entryStatus = "GPSentered";
            entryState = 1;
            blueOn();
          }
          // Serial.println(F("[locationTracker] checking completed")); // discovered whitespace 19 Nov

        // if (! pubTracker.publish("####[pubTracker] ESP32 received lastread in ELSE block ####")) {
        //   Serial.println(F("Failed"));
        // } else {
        //   Serial.println(F("####[pubTracker] SUCCESS! ESP32 received lastread in ELSE block ####"));
        // } // end of else
      } // end of if..else (biggest block)
      
      ets = get_timestamp();

      Serial.println("EEPROM write started");
      EEPROM.write(entryStatusAddress, entryState);
      writeStringToEEPROM(entryTimestampAddress, ets);

      /*
      // this method was discovered on 15 Nov 2023
      // using char array datatype and itoa method

      char cstr[10];
      itoa(entryState, cstr, 10);
      String responseStr = "ESP32 ack response | entryState = ";
      responseStr.concat(cstr);
      Serial.print("cstr:");
      Serial.println(cstr);
      */
      
      String responseStr = "ESP32 ACK | entryState = ";
      responseStr.concat(String(entryState));
      responseStr.concat(" | ");   
      responseStr.concat(entryStatus);   
      const char* eeprom_ack_str = responseStr.c_str();

      // Serial.print("eeprom_ack_str:");
      // Serial.println(eeprom_ack_str);

      if (! pubTracker.publish(eeprom_ack_str)) {
          Serial.println(F("Failed"));
      } else {
          Serial.println(F(eeprom_ack_str));
      }
      delay(100);

      responseStr = "Total Elapse for ";
      if(entryState) // this should be the opposite of current state / to reflect the previous state
        responseStr.concat("GPS Exited");
      else
        responseStr.concat("GPS Entered");
      responseStr.concat(" | ");   
      responseStr.concat(e_elapse);   
      eeprom_ack_str = responseStr.c_str();

      // Serial.print("eeprom_ack_str:");
      // Serial.println(eeprom_ack_str);

      if (! pubTracker.publish(eeprom_ack_str)) {
          Serial.println(F("Failed"));
      } else {
          Serial.println(F(eeprom_ack_str));
      }
      delay(100);
      Serial.println("EEPROM write ended");
      // read_eeprom();
    } // end of locationTracker
  } //  end of MQTT read subscription blocking loop

  countElapse(); // called to update entry / exit elapse timer

  lcd.clear();
  if(elapseSec % 2 == 0){
    lcd.setCursor(0, 0); // row 1, column 0
    lcd.print("Last "+ entryStatus); // default_hostname
    lcd.setCursor(0, 1); // row 1, column 0
    lcd.print(ets);
  }
  else{
    // display_uptime_top_row();
    lcd.setCursor(0, 0); // row 1, column 0
    if(entryState == 1 || entryState == 3)
      lcd.print("IN for:"+ e_elapse); // in elapse
    else
      lcd.print("OUTfor:"+ e_elapse); // out elapse

    lcd.setCursor(0, 1); // row 1, column 0
    // lcd.print("Status:"+ entryStatus); // default_hostname
    if(entryState == 1 || entryState == 3)
      lcd.print("SirYazid is here"); 
    else
      lcd.print("SirYazid at home"); 

  }

} // end of void loop for MQTT

void MQTT_connect() {
  int8_t ret;
  // Stop if already connected.
  if (mqtt.connected()) {
    return;
  }

 Serial.print("Connecting to Adafruit IO");

 /*
 -1 = Error connecting to server 
 1 = Wrong protocol 
 2 = ID rejected 
 3 = Server unavailable 
 4 = Bad username or password 
 5 = Not authenticated 
 6 = Failed to subscribe 
 Use connectErrorString() to get a printable string version of the error.
 
 */

  uint8_t retries = 5;
  while ((ret = mqtt.connect()) != 0) { // connect will return 0 for connected
       Serial.println(mqtt.connectErrorString(ret));
       lcd.clear();
        lcd.setCursor(0, 0); // row 1, column 0
        lcd.print("AdafruitIO Try "+String(retries)); // default_hostname
        lcd.setCursor(0, 1); // row 1, column 0
        // lcd.print("-Not Authorized-");
        lcd.print(mqtt.connectErrorString(ret));


       Serial.println("Retrying Adafruit connection in 1 seconds...");
       mqtt.disconnect();
       delay(1000);  
       retries--;
       if (retries == 0) {
        //  while (1);
        Serial.println("Giving up MQTT...restarting now");
        lcd.clear();
        lcd.setCursor(0, 0); // row 1, column 0
        lcd.print("Adafruit IO Conn"); // default_hostname
        lcd.setCursor(0, 1); // row 1, column 0
        lcd.print("-Restarting ESP-");

        delay(1000);
        ESP.restart();
       }
  }
  Serial.println("");
  Serial.println("MQTT Connected!");
  Serial.println("****************** Adafruit IO is Connected! ****************** ");
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
  if(strcmp(macStr, client9_cchar) == 0) 
  {
    Serial.println("Client ESP32S2m1");
  }
  // Serial.println("Unknown Client! Check client's MAC");


  memcpy(&incomingReadings, incomingData, sizeof(incomingReadings));

  // float v_2dpf = round_2dp(incomingReadings.volt);
  // float i_2dpf = round_2dp(incomingReadings.amps);

  String v2dp = String(incomingReadings.volt,2);
  String i2dp = String(incomingReadings.amps,2);
  float v_2dpf = v2dp.toFloat();
  float i_2dpf = i2dp.toFloat();
  // must use double quote for the json label 27.08.2022
  board["d"] = incomingReadings.id; 
  board["#"] = String(incomingReadings.readingId);
  board["v"] = v_2dpf; 
  board["a"] = i_2dpf;  
  board["t"] = incomingReadings.temp; 
  board["h"] = incomingReadings.humi; 
  board["m"] = incomingReadings.mois; 
  board["r"] = incomingReadings.rain; 


  if(incomingReadings.id == "ESP32C3-1"){
    temp1 = incomingReadings.temp;
    humi1 = incomingReadings.humi;
    moist1 = incomingReadings.mois;
    rain1 = incomingReadings.rain;
    volt1 = round_2dp(incomingReadings.volt);
    amps1 = round_2dp(incomingReadings.amps);
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
    volt2 = round_2dp(incomingReadings.volt);
    amps2 = round_2dp(incomingReadings.amps);
    read_id2 = incomingReadings.readingId;
    dt2 = get_timestamp();
    jsonString2 = JSON.stringify(board);
  }
  if(prev_read_id2 != read_id2){
    prev_read_id2 = read_id2;
  } 

  if(incomingReadings.id == "ESP32S2m1"){ // condemned C3-3 changed to ESP32S2m1
    temp3 = incomingReadings.temp;
    humi3 = incomingReadings.humi;
    moist3 = incomingReadings.mois;
    rain3 = incomingReadings.rain;
    volt3 = round_2dp(incomingReadings.volt);
    amps3 = round_2dp(incomingReadings.amps);
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
    volt4 = round_2dp(incomingReadings.volt);
    amps4 = round_2dp(incomingReadings.amps);
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
    volt6 = round_2dp(incomingReadings.volt);
    amps6 = round_2dp(incomingReadings.amps);
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
    volt7 = round_2dp(incomingReadings.volt);
    amps7 = round_2dp(incomingReadings.amps);
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
    volt8 = round_2dp(incomingReadings.volt);
    amps8 = round_2dp(incomingReadings.amps);
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

    // Register peer (peer here is going to be client-peerInfo9) CLIENT ID S2m1
  esp_now_peer_info_t peerInfo9;
  memset(&peerInfo9, 0, sizeof(peerInfo9)); // https://github.com/espressif/arduino-esp32/issues/6029
  memcpy(peerInfo9.peer_addr, client9_mac, 6);
  peerInfo9.encrypt = false;
  
  // Add peer S2 Mini 1
  if (esp_now_add_peer(&peerInfo9) != ESP_OK){
    Serial.println("Failed to add peer client ID S2m1 (ESP32S2m1)");
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
  // timer.setInterval(1000L, get_weather);

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

    Serial.printf("\n\nConnecting to %s\n\n", ssidstr);
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
                lcd.print(ssidstr); // default_hostname
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
                lcd.print(ssidstr);
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
      Serial.println("Syncing RTC via NTP now...");
      timeClient.begin();
      timeClient.setTimeOffset(28800);
      timeClient.update();
      sync_rtc_ntp();
      Serial.println("Syncing RTC via NTP complete. RTC is now up-to-date!");
    }
  
    if(GROVE_LCD_AVAILABLE || I2C_LCD_AVAILABLE){
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("-WiFi Connected-");
      lcd.setCursor(0,1);
      lcd.print(ssidstr);
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

void countElapse(){
  /////////// Entry or Exit Duration Elapse Counter Starts ///////////
  if(entryState == 1 || entryState == 3){
    elapseExit = 0; // reset elapseExit upon entry
    elapseEntry += 1; // increment elapseEntry
    elapseSec = elapseEntry;
    e_h = elapseEntry / 3600;
    e_m = (elapseEntry % 3600) / 60;
    e_s = elapseEntry % 60;
  } else{
    elapseExit += 1; // increment elapseExit
    elapseEntry = 0; // reset elapseEntry upon exit
    elapseSec = elapseExit;
    e_h = elapseExit / 3600;
    e_m = (elapseExit % 3600) / 60;
    e_s = elapseExit % 60;
  }

  if(e_h < 10)  e_HourStr = "0"+String(e_h);
  else              e_HourStr = String(e_h);
  if(e_m < 10)   e_MinStr = "0"+String(e_m);
  else              e_MinStr = String(e_m);
  if(e_s < 10)   e_SecStr = "0"+String(e_s);
  else              e_SecStr = String(e_s);

  if(e_s == 0){
    e_elapse = e_HourStr +":"+ e_MinStr +":"+ e_SecStr;  
    write16bitIntoEEPROM(elapseAdd, elapseSec);
    // e_elapse = e_HourStr +":"+ e_MinStr +":"+ e_SecStr;  
    // writeStringToEEPROM(elapseAdd,e_elapse);
  }else{
    e_elapse = e_HourStr +":"+ e_MinStr +":"+ e_SecStr;  
  }  
  /////////// Entry or Exit Duration Elapse Counter Ends ///////////

} // end of countElapse for GPSentered and GPSexited


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
    String etaHourStr, etaMinStr, etaSecStr;
    if(etaHour < 10)  etaHourStr = "0"+String(etaHour);
    else              etaHourStr = String(etaHour);
    if(etaMin < 10)   etaMinStr = "0"+String(etaMin);
    else              etaMinStr = String(etaMin);
    if(etaSec < 10)   etaSecStr = "0"+String(etaSec);
    else              etaSecStr = String(etaSec);

    etaDS = etaHourStr +":"+ etaMinStr +":"+ etaSecStr;  
    int onlineHour = currentHour + SLEEP_HOUR - 24; // calc reconnection time

    String onHour, onMin;
    if(onlineHour < 10) onHour = "0"+String(onlineHour);
    else                onHour = String(onlineHour);      
    if(currentMin < 10) onMin = "0"+String(currentMin);
    else                onMin = String(currentMin);

    Blynk.virtualWrite(V3, "DS starts in T-"+ etaDS);
    // if currentHour similar to WAKEUP_HOUR, then skip the logic
    if((currentHour >= setSleepHour && currentMin >= setSleepMin) || (currentHour >= 0 && currentHour < WAKEUP_HOUR)){
      Serial.println("Going to sleep now. Pushing Deep Sleep Timestamp of " + dateTime);
      Serial.println("Will be back online at " + String(onlineHour) + ":" + String(currentMin));
      Blynk.virtualWrite(V3, "DS started at "+ dateTime);
      delay(1000);
      Blynk.virtualWrite(V3, "Back online at " + (onHour) + ":" + (onMin));
      Serial.flush(); 
      lcd.clear();
      lcd.setCursor(0, 0); // row 0, column 0
      lcd.print("-Back Online at:");
      lcd.setCursor(0, 1); // row 0, column 0
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
} // end of get_timestamp()


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

void onBacklight(){
  #ifdef REGULAR_I2C_LCD
    lcd.backlight();
  #endif
}

void offBacklight(){
  #ifdef REGULAR_I2C_LCD
    lcd.noBacklight();
  #endif
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

void clear_eeprom(){
  for (int i = 0; i < EEPROM_SIZE; i++)
  {
    EEPROM.write(i, 255);
  }
  Serial.println("[EEPROM REFORMATTED]");
}

void read_eeprom(){

  Serial.println(" [READ AGAIN] bytes read from Flash:");
  for (int i = 0; i < EEPROM_SIZE; i++)
  {
    if(i >= 20 && i <= 34){
      Serial.print("[+][IFTTT] Disconnected Timestamp String Memory "); 
      Serial.print(i); Serial.print(" = ");
      Serial.print(i,HEX); Serial.print(" ASCII DEC:");
    }

  // #define entryStatusAddress      0x23 // HEX 0x23 (DEC 35)
  // #define entryTimestampAddress   0x24 // HEX 0x24 (DEC 36 - 50)

    if(i == 35){
      Serial.print("[+][IFTTT] entryState code at location "); 
      Serial.print(i); Serial.print(" = ");
      Serial.print(i,HEX); Serial.print(" ASCII DEC:");
    }

    if(i >= 36 && i <= 50){ // 0x31 until 0x3F
      Serial.print("[+] entryTimestamp at location "); 
      Serial.print(i); Serial.print(" = ");
      Serial.print(i,HEX); Serial.print(" ASCII DEC:");
    }
    if(i>50){
      Serial.print("[+][IFTTT] Unallocated location "); 
      Serial.print(i); Serial.print(" = ");
      Serial.print(i,HEX); Serial.print(" ASCII DEC:");
    }
    // else if(i== 12 || i==13){
    //     Serial.print("WiFi Retry Count Memory "); Serial.print(i);   Serial.print(":");
    // }
    // else if(i== 15 || i==16){
    //     Serial.print("Restart Count Memory "); Serial.print(i);   Serial.print(":");
    // }else if (i== 18 || i==19){
    //     Serial.print("Disconnection Count Memory "); Serial.print(i);   Serial.print(":");
    // }else{
    //     Serial.print("R "); Serial.print(i);   Serial.print(":");
    // }
    Serial.print(byte(EEPROM.read(i))); Serial.println();
  }
  Serial.println();


}

void init_eeprom(){

  if (!EEPROM.begin(EEPROM_SIZE))
  {
    Serial.println("failed to initialise EEPROM"); delay(1000000);
  }

//   Serial.println("[EEPROM] bytes read from Flash:");
//   for (int i = 0; i < EEPROM_SIZE; i++)
//   {
//     if(i >= 20 && i <= 34){
//       Serial.print("[IFTTT] Disconn TS str Mem "); 
//       Serial.print(i); Serial.print(" = ");
//       Serial.print(i,HEX); Serial.print(" ASCII DEC:");
//     }

// // #define entryStatusAddress      0x23 // HEX 0x23 (DEC 35)
// // #define entryTimestampAddress   0x24 // HEX 0x24 (DEC 36 - 50)

//     if(i == 35){
//       Serial.print("[IFTTT] entryState code at location "); 
//       Serial.print(i); Serial.print(" = ");
//       Serial.print(i,HEX); Serial.print(" ASCII DEC:");
//     }

//     if(i >= 36 && i <= 50){ // 0x31 until 0x3F
//       Serial.print("[IFTTT] entryTimestamp at location "); 
//       Serial.print(i); Serial.print(" = ");
//       Serial.print(i,HEX); Serial.print(" ASCII DEC:");
//     }
//     // 15:35:02 11-17 (ascii char) (15 mem blocks including NULL char \0)
//     // 14(shift out) 49 53 58 51 53 58 48 50 32 49 49 45 49 55
//     if(i>50){
//       Serial.print("Unallocated location "); 
//       Serial.print(i); Serial.print(" = ");
//       Serial.print(i,HEX); Serial.print(" ASCII DEC:");
//     }
    
//     // else if(i== 12 || i==13){
//     //     Serial.print("WiFi Retry Count Memory "); Serial.print(i);   Serial.print(":");
//     // }
//     // else if(i== 15 || i==16){
//     //     Serial.print("Restart Count Memory "); Serial.print(i);   Serial.print(":");
//     // }else if (i== 18 || i==19){
//     //     Serial.print("Disconnection Count Memory "); Serial.print(i);   Serial.print(":");
//     // }else{
//     //     Serial.print("R "); Serial.print(i);   Serial.print(":");
//     // }
//     if(byte(EEPROM.read(i))==14) Serial.print("SHIFT OUT:");
//     Serial.print(byte(EEPROM.read(i))); Serial.println();
//   }
//   Serial.println();
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
  // ina219_A.setCalibration_32V_2A();
  ina219_A.setCalibration_16V_400mA();
}

float prev_mA = -1;
bool sampleTaken = false;
void getINA219(){
  shuntvoltage = ina219_A.getShuntVoltage_mV(); // measured across Vin+ & Vin-
  busvoltage = ina219_A.getBusVoltage_V(); // measured at Vin+ only
  current_mA = ina219_A.getCurrent_mA();
  if(!sampleTaken){
    prev_mA = current_mA;
    sampleTaken = true;
  }
  // to ensure the current_mA is only within +/- 50mA to be seen as valid value (no big jump)
  if((current_mA > prev_mA-50) && (current_mA < prev_mA+50)){
    prev_mA = current_mA;
  }else{
    current_mA = prev_mA; // use the prev readout if the jump in value is too big (glitches)
  }
}

void i2c_scan() {
  byte error, address, nDevices = 0;
  Serial.println("Scanning...");
  // Wire.setClock(125000); // discovered 27.02.2023 at Berlian | disabled 3.03.2023 Friday MKE2 (issue fixed)
  for(address = 1; address < 127; address++ ) {
    Wire.beginTransmission(address);
    delay(1); // re-enabled 3.03.2023 Friday MKE2 (issue fixed) | kind of fixing the slow scanning issue 03.03.2023
    error = Wire.endTransmission();
    // Serial.printf("Address: 0x%02x \t Error: %d\n", address, error); // added 27.02.2023 at Berlian

    if (error == 0) {
    //   Serial.print("------------------------------- I2C device found at address 0x");
    //   if (address<16) {
    //     Serial.print("0");
    //   }
    //   Serial.println(address,HEX);
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
    if(INA219_AVAILABLE) getINA219();
    if(ADS1115_AVAILABLE) 
      read_ads1115(); //   Blynk.virtualWrite(V18, ads_readout) happening inside func
    else{
      // Blynk.virtualWrite(V18, "ADS1115 Not Connected"); // before adding sonar SR04-M2
      // Blynk.virtualWrite(V18, "Water Level = " + String(get_water_level_cm())+" cm @ " + String(waterLvlPercent)+"%");   
      Blynk.virtualWrite(V44,get_water_level_cm());
      Blynk.virtualWrite(V43,waterLvlPercent);
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
    #ifdef ESP32DEV_5
      Blynk.virtualWrite(V12, "S2m1 = " + jsonString3); // ESP32S2m1 taking over ESP32C3-3 (condemned)
      Blynk.virtualWrite(V15, "C3-6 = " + jsonString6); // taking over disconnected TS
    #elif defined ESP32S2_3
      Blynk.virtualWrite(V12, "C3-4 = " + jsonString4); // 
      Blynk.virtualWrite(V15, "C3-7 = " + jsonString7); //
    #else
    #endif    
    Blynk.virtualWrite(V34, get_ambient_temp()); // gonna change this to V30 - 21/6/2023

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
            lcd.print("S2m1 #"+String(read_id3)+" V:" + String(volt3)); // print connected SSID
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
  Blynk.syncVirtual(V20,V21,V22,V23,V24,V25,V26,V40,V41);
  Blynk.virtualWrite(V2, WiFi.localIP().toString());
  Blynk.virtualWrite(V3, restart_ts); 
  Blynk.virtualWrite(V5, restartCounter);
  Blynk.virtualWrite(V8, ssid);
  String disconn_ts_str = readStringFromEEPROM(disconnectTS__Address);
  Serial.print("|||||||||||||||||||||||||||||||||| [IFTTT/Blynk Restart] The disconnection timestamp from EEPROM: ");
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

// LED backlit control
BLYNK_WRITE(V40){
  int val = param.asInt();
  if(val)       onBacklight();
  else          offBacklight();
}

// deep sleep control
BLYNK_WRITE(V41){
  int val = param.asInt();
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

float prev_dist = 0;
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
  if(distance_cm < 40) distance_cm = 40;
  if(distance_cm > 90) distance_cm = 90;  

  // We print the distance on the serial port
  // Serial.print("Distance (cm): ");
  // Serial.println(distance_cm);
  // if(distance_cm > (prev_dist - 5) && distance_cm < (prev_dist + 5)){
  if(distance_cm != 40 && distance_cm != 90){
    prev_dist = distance_cm;
    waterLvlPercent = map(distance_cm,90,40,0,100);  
    return distance_cm;    
  }else{
    waterLvlPercent = map(prev_dist,90,40,0,100);  
    return prev_dist;
  }
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

double round_2dp(double x){
  return (int)(x * 100 + 0.5) / 100.0;
}