#define BLYNK_PRINT Serial // Defines the object that is used for printing
// #define BLYNK_DEBUG        // Optional, this enables more detailed prints

#define BLYNK_TEMPLATE_ID "TMPLLcUZS8pw"

#define AFS2 // options: AFS/AFS2

#ifdef AFS
  #define BLYNK_DEVICE_NAME "AFS"
  #define BLYNK_AUTH_TOKEN "3G4XbLzWHurLKwzeAeKQZH7QttvcM9gR"
#else
  #define BLYNK_DEVICE_NAME "AFS2"
  #define BLYNK_AUTH_TOKEN "WMPQFiXeWmh7xHHUsngi8oyIHO4bG47D"
#endif
//#define USE_RTC

#include <esp_now.h>
#include <WiFi.h>
#include <ESP32Ping.h>
#include <WiFiClient.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

#include "ESPAsyncWebServer.h"
#include <Arduino_JSON.h>
#include <BlynkSimpleEsp32.h>
#include <Wire.h>
#include "rgb_lcd.h"
#include "DS1307.h"
#include "uptime_formatter.h"
#include "uptime.h"
#include "EEPROM.h"
#include <Adafruit_INA219.h>

TaskHandle_t Task1; // ESPNOW
TaskHandle_t Task2; // BLYNK
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

#define EEPROM_SIZE 32
#define relay_in1   17
#define relay_in2   25
#define relay_in3   27
#define relay_in4   16
#define state0_vpin 30
#define state1_vpin 31
#define state2_vpin 32
#define state3_vpin 33
#define CORE_0      0x00
#define CORE_1      0x01
#define restartCounterAddress   0x0F // 15 : 1 byte
#define ESP_RST_COUNTER_ADDR    0x10 // 16 : 1 byte

byte restartCounter;      // value will be loaded from EEPROM
byte prev_restartCounter;
String restart_ts = "None";

// Replace with your network credentials (STATION)
char auth[] = BLYNK_AUTH_TOKEN;
const char* ssid = "NPRDC CELCOM M3";
const char* pass = "nprdc1234";
const char* remote_host = "blynk.cloud";

int board_id = 0;

String jsonString1 = "None";
String dt1 = "None";
float temp1 = 0;
float humi1 = 0;
int random1 = 0;
int read_id1 = 0;

String jsonString2 = "None";
String dt2 = "None";
float temp2 = 0;
float humi2 = 0;
int random2 = 0;
int read_id2 = 0;

float shuntvoltage;
float busvoltage;
float current_mA; 

unsigned long previousMillis = 0;   // Stores last time temperature was published
unsigned int interval = 60000;      // very long delay just to free up more CPU time
bool esp_now_initialized = false;

// MAC Address of the receiver #1 => 9c:9c:1f:c5:94:24 (ESP01-client-1)
uint8_t client1_mac[] = {0x9C, 0x9C, 0x1F, 0xC5, 0x94, 0x24};
String client1_mac_string = "9c:9c:1f:c5:94:24";
const char* client1_cchar = client1_mac_string.c_str();

// MAC Address of the receiver #2 => 9c:9c:1f:e3:85:3c (ESP01-client-2)
uint8_t client2_mac[] = {0x9C, 0x9C, 0x1F, 0xE3, 0x85, 0x3C};
String client2_mac_string = "9c:9c:1f:e3:85:3c";
const char* client2_cchar = client2_mac_string.c_str();

// Structure example to receive data
// Must match the sender structure
typedef struct struct_message {
  int id;
  float temp;
  float humi;
  int randomNum;
  unsigned int readingId;
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
BlynkTimer  timer;
DS1307      _clock;
rgb_lcd     lcd;
WiFiUDP     ntpUDP;
NTPClient   timeClient(ntpUDP, "pool.ntp.org", 3600, 60000);
Adafruit_INA219     ina219_A;

// Variables to save date and time
String formattedDate;
String dayStamp;
String timeStamp;

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

String get_timestamp(){
  timeClient.update();
  formattedDate = timeClient.getFormattedDate();
  // Extract date
  int splitT = formattedDate.indexOf("T");
  dayStamp = formattedDate.substring(0, splitT);
  String dateTime = timeClient.getFormattedTime() + " " + dayStamp;
  return dateTime;
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
  ina219_A.setCalibration_16V_1_6A(); // added 26.3.2021 by clumzyZidz
}

void getINA219(){
  shuntvoltage = ina219_A.getShuntVoltage_mV(); // measured across Vin+ & Vin-
  busvoltage = ina219_A.getBusVoltage_V(); // measured at Vin+ only
  current_mA = ina219_A.getCurrent_mA();
}

void blynk_tasks(){
  lcd.clear();
  String dateTime = get_timestamp();
  display_uptime_top_row();
  lcd.setCursor(0,1);
  lcd.print(dateTime);
  Blynk.virtualWrite(V10,uptime_formatter::getUptime());
  Blynk.virtualWrite(V11, dateTime);
  Blynk.virtualWrite(V13,jsonString1 + " | Received at: " + dt1);
  Blynk.virtualWrite(V20,temp1);
  Blynk.virtualWrite(V21,humi1);
  Blynk.virtualWrite(V22,read_id1);
  Blynk.virtualWrite(V23,random1);

  Blynk.virtualWrite(V14,jsonString2 + " | Received at: " + dt2);
  Blynk.virtualWrite(V24,temp2);
  Blynk.virtualWrite(V25,humi2);
  Blynk.virtualWrite(V26,read_id2);
  Blynk.virtualWrite(V27,random2);
  
  Blynk.virtualWrite(V40,busvoltage);
  Blynk.virtualWrite(V41,shuntvoltage);
  Blynk.virtualWrite(V42,current_mA);



}

BLYNK_CONNECTED() {
  timeClient.setTimeOffset(28800);
  restart_ts = get_timestamp();
  Blynk.syncVirtual(V0, V1, V2, V3);
  Blynk.virtualWrite(V12,WiFi.localIP().toString());
  Blynk.virtualWrite(V16, restart_ts); 
  Blynk.virtualWrite(V15,restartCounter);
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

  if(esp_now_initialized){
    sendControl.control = pinValue;
    //Send message via ESP-NOW
    esp_err_t result = esp_now_send(client2_mac, (uint8_t *) &sendControl, sizeof(sendControl));
    if (result == ESP_OK) {
      Serial.println("Sent with success from BLYNK_WRITE(V2)");
    }
    else {
      Serial.println("Error sending the data");
    }
  }
}
BLYNK_WRITE(V3)
{
  int pinValue = param.asInt(); // assigning incoming value from pin V1 to a variable
  digitalWrite(relay_in4,pinValue);
  Blynk.virtualWrite(state3_vpin, digitalRead(relay_in4));
  
  if(esp_now_initialized){
    sendControl.control = pinValue;
    //Send message via ESP-NOW
    esp_err_t result = esp_now_send(client1_mac, (uint8_t *) &sendControl, sizeof(sendControl));
    if (result == ESP_OK) {
      Serial.println("Sent with success from BLYNK_WRITE(V3)");
    }
    else {
      Serial.println("Error sending the data");
    }
  }
}

BLYNK_WRITE(V4){
  int pinValue = param.asInt();
  if(pinValue){
    lcd.clear();
    lcd.setCursor(0, 0); // row 0, column 0
    lcd.print("FORCE RESTART");
    lcd.setCursor(0, 1); // row 1, column 0
    lcd.print("REQUESTED"); 
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    ESP.restart();
  }
}

BLYNK_WRITE(V5){
  int pinValue = param.asInt();
  if(pinValue){
    Serial.println("CLEARING MEMORY#");
    lcd.setCursor(0, 0); // row 0, column 0
    lcd.print("Clearing 32Bytes");
    lcd.setCursor(0, 1); // row 1, column 0
    lcd.print("PLEASE WAIT "); 

    for (int w=0; w < EEPROM_SIZE ; w++){
      EEPROM.write(w, 255);
      lcd.setCursor(0, 1); // row 1, column 0
      lcd.print("PLEASE WAIT "+String(w));
      Serial.print("W "); Serial.print(w);   Serial.print(":");  Serial.println("255");  
    }
    EEPROM.commit();
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    lcd.clear();
    lcd.setCursor(0, 0); // row 0, column 0
    lcd.print("MEMORY CLEARED");
    lcd.setCursor(0, 1); // row 1, column 0
    lcd.print("RETURNING NOW"); 
  }
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
  // Serial.println(macStr);
  if(strcmp(macStr, client1_cchar) == 0) Serial.println("Client ID 1");
  else if(strcmp(macStr, client2_cchar) == 0) Serial.println("Client ID 2");
  else Serial.print(".");

  memcpy(&incomingReadings, incomingData, sizeof(incomingReadings));

  board["id"] = incomingReadings.id;
  board["temperature"] = incomingReadings.temp;
  board["humidity"] = incomingReadings.humi;
  board["random"] = incomingReadings.randomNum;
  board["readingId"] = String(incomingReadings.readingId);


  if(incomingReadings.id == 1){
    temp1 = incomingReadings.temp;
    humi1 = incomingReadings.humi;
    random1 = incomingReadings.randomNum;
    read_id1 = incomingReadings.readingId;
    dt1 = get_timestamp();
    jsonString1 = JSON.stringify(board);
  }
  else if(incomingReadings.id == 2){
    temp2 = incomingReadings.temp;
    humi2 = incomingReadings.humi;
    random2 = incomingReadings.randomNum;
    read_id2 = incomingReadings.readingId;
    dt2 = get_timestamp();
    jsonString2 = JSON.stringify(board);
  }
  else{
    Serial.println("No payload received");
  }
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
  // coming from client-side code end ----------------------------------------------------------

  // leave this empty to reserve more CPU resources for other intensive tasks    
  for(;;){} // empty forever loop (no need to do anything inside here otherwise it will cost some CPU time)
} // end of FreeRTOS handler

void BLYNK_HandlerTask(void * pvParameters) 
{
  #ifdef USE_RTC
    timer.setInterval(1000L, printTimeRTC);
  #else
    timer.setInterval(1000L, blynk_tasks);
    timer.setInterval(1000L, getINA219);

  #endif
  Serial.print("BLYNK_HandlerTask running on core ");
  Serial.println(xPortGetCoreID());
  for(;;){    
    Blynk.run();
    timer.run();
  }
} // end of FreeRTOS handler

#define FAST_DELAY 1000
void check_restart_count(){
  if(restartCounter==255 ){
    lcd.print("IT'S NEW NODE");
    lcd.setCursor(0, 1); // row 1, column 0
    lcd.print("Reset Rc Counter"); // Reboot 000 times
    vTaskDelay(FAST_DELAY / portTICK_PERIOD_MS);
    lcd.clear();

    restartCounter = 0;
    EEPROM.write(restartCounterAddress, restartCounter);
    EEPROM.commit();
    vTaskDelay(3.3 / portTICK_PERIOD_MS); // EEPROM needs 3.3ms to write

  }
  else{
      Serial.println("just a normal reboot or restart");
      lcd.setCursor(0, 0); 
      lcd.print("REBOOTED NODE");
      lcd.setCursor(0, 1); // row 1, column 0
      lcd.print("Restarted "+String(restartCounter) + " tms");
      vTaskDelay(FAST_DELAY / portTICK_PERIOD_MS);
      lcd.clear();
      
      prev_restartCounter = restartCounter; // for the comparison later on
      restartCounter += 1;  // increment rst count by 1 for each reboot
      Serial.printf("the current restart count is %d\n", restartCounter);
      lcd.setCursor(0, 0); 
      lcd.print("--WELCOME BACK--"); 
      lcd.setCursor(0, 1); // row 1, column 0
      lcd.print("Current RST "+String(restartCounter) + " times"); // load current rst count
      vTaskDelay(FAST_DELAY / portTICK_PERIOD_MS);
      lcd.clear();

      EEPROM.write(restartCounterAddress, restartCounter);
      EEPROM.commit();
      vTaskDelay(3.3 / portTICK_PERIOD_MS); // EEPROM needs 3.3ms to write          
  }
}

void init_eeprom(){

  if (!EEPROM.begin(EEPROM_SIZE))
  {
    lcd.clear();
    lcd.setCursor(0, 0); 
    lcd.print("-CRITICAL ERROR-");
    lcd.setCursor(0, 1); 
    lcd.print("-EEPROM  FAILED-");
    vTaskDelay(1000000 / portTICK_PERIOD_MS);
    // Serial.println("failed to initialise EEPROM"); delay(1000000);

  }

  Serial.println(" bytes read from Flash:");
  for (int i = 0; i < EEPROM_SIZE; i++)
  {
    Serial.print("R "); Serial.print(i);   Serial.print(":");
    Serial.print(byte(EEPROM.read(i))); Serial.println();
  }
  Serial.println();
}


void setup() {
  // Initialize Serial Monitor
  Serial.begin(115200);

  lcd.begin(16, 2);
  lcd.createChar(1, wave_right); // create block character
  lcd.createChar(2, wave_left); // create block character
  lcd.createChar(3, right_arrow); // create block character
  lcd.createChar(4, degree_symbol); // create block character

  init_eeprom();

  restartCounter = EEPROM.read(restartCounterAddress);
  check_restart_count();

  pinMode(relay_in1,OUTPUT);
  pinMode(relay_in2,OUTPUT);
  pinMode(relay_in3,OUTPUT);
  pinMode(relay_in4,OUTPUT);

  /*

  // Set the device as a Station and Soft Access Point simultaneously
  WiFi.mode(WIFI_AP_STA);
  Serial.print("Connecting to ");
  Serial.println(ssid);

  lcd.setCursor(0,0);
  lcd.print("Welcome to AFS");
  lcd.setCursor(0,1);
  lcd.print("Connecting WiFi");
  delay(1000);
  WiFi.mode(WIFI_AP_STA);
  WiFi.begin(ssid, pass);

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
          column_index_right  = init_index_right; 
          column_index_left   = init_index_left;
          lcd.clear();
          if(millis()/1000 % 2 == 0){
              lcd.setCursor(0, 0); // row 1, column 0
              lcd.print("Connecting WiFi");  
          }
          else{
              lcd.setCursor(0, 0); // row 1, column 0
              lcd.print(String(ssid));

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

  #ifdef USE_RTC
    _clock.begin();
    _clock.fillByYMD(2021,12,24);
    _clock.fillByHMS(12,30,30);
    _clock.fillDayOfWeek(FRI);
    _clock.setTime();
  #endif

  Serial.print("Station IP Address: ");
  Serial.println(WiFi.localIP());
  Serial.print("Wi-Fi Channel: ");
  Serial.println(WiFi.channel());
  Serial.print("ESP Board MAC Address:  ");
  Serial.println(WiFi.macAddress());
  */

// /Users/zidz/Documents/Arduino/libraries/Blynk/src/Adapters/BlynkArduinoClient.h
// /Users/zidz/Documents/Arduino/libraries/Blynk/src/Blynk/BlynkProtocol.h (1 April 2022)
// /Users/zidz/Documents/Arduino/libraries/Blynk/src/BlynkSimpleEsp32.h (1 April 2022)
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Connecting Blynk");
  lcd.setCursor(0,1);
  lcd.print("blynk.cloud:80");
// to configure the begin timeout, edit the file at path above
  Blynk.begin(auth, ssid, pass);
  Serial.print("[BLYNK WiFi Handler Main Server Code] WiFi connected with IP ");  
  Serial.println(WiFi.localIP());
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("!WiFi Connected!");
  lcd.setCursor(0,1);
  lcd.print("-Using NTP Time-");
  delay(1000);
  
  timeClient.begin();
  timeClient.setTimeOffset(28800);
  initINA219();

  // timer.setInterval(100, handle_server_event);
  // timer.setInterval(15000L, Get_Ping);

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


}

void loop() {
  // do not run Blynk here! ESP can't manage it well
}

