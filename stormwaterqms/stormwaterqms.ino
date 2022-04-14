#define BLYNK_PRINT Serial // Defines the object that is used for printing
// #define BLYNK_DEBUG        // Optional, this enables more detailed prints

#define BLYNK_TEMPLATE_ID "TMPLLpuw4V7u"
#define BLYNK_DEVICE_NAME "Stormwater QMS Template"
#define BLYNK_AUTH_TOKEN "0lTHr8-8wkZzqMklavhHXRf7tb85sOW4"

//#define USE_RTC
#include "GroveBase-ESPDuino32-Mapping.h"
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
#include<ADS1115_WE.h> 
#define I2C_ADDRESS 0x48

#define lcd_backlight GROVE_D13

TaskHandle_t Task1; // ESPNOW
TaskHandle_t Task2; // BLYNK
String classTSS                   = "None";

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
const char* ssid = "NPRDC CELCOM M2";
const char* pass = "nprdc1234";
const char* remote_host = "blynk.cloud";

float shuntvoltage;
float busvoltage;
float current_mA; 

unsigned long previousMillis = 0;   // Stores last time temperature was published
unsigned int interval = 60000;      // very long delay just to free up more CPU time
bool esp_now_initialized = false;

JSONVar     board;
BlynkTimer  timer;
DS1307      _clock;
rgb_lcd     lcd;
WiFiUDP     ntpUDP;
NTPClient   timeClient(ntpUDP, "pool.ntp.org", 3600, 60000);
Adafruit_INA219     ina219_A;
ADS1115_WE adc = ADS1115_WE(I2C_ADDRESS);
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

#define REFERENCE_1500mV 1500
#define CALIBRATED_NEUTRAL_V 1500
#define CALIBRATED_ACID_V 2032.44

float getPH(){
    float voltage = readChannel(ADS1115_COMP_1_GND)*1000; // TODO: coming from ADS1115   
    // voltage = voltage - offsetVph; // disable this line if supply to pH sensor is 5.0V exact
    // phValue = ph.readPH(voltage,fluidTemp);  // convert voltage to pH with temperature compensation
    float slope = (7.0-4.0)/((CALIBRATED_NEUTRAL_V - REFERENCE_1500mV)/3.0 - (CALIBRATED_ACID_V - REFERENCE_1500mV)/3.0);  // two point: (_neutralVoltage,7.0),(_acidVoltage,4.0)
    float intercept =  7.0 - slope*(CALIBRATED_NEUTRAL_V - REFERENCE_1500mV)/3.0;
    float phValue = slope*(voltage-REFERENCE_1500mV)/3.0+intercept;
    return phValue;
  }

float _map(float x, float in_min, float in_max, float out_min, float out_max){
    return float((x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min);
}

float prevMax1 = 0.0;
String class1_use_ws1 = "Water Supply I - No treatment necessary";
String class1_use_f1  = "Fishery I - Very sensitive aquatic species";

String class2_use_ws2 = "Water Supply II - Conventional treatment required";
String class2_use_f2  = "Fishery II - Sensitive aquatic species";

String class3_use_ws3 = "Water Supply III - Extensive treatment required";
String class3_use_f3  = "Fishery III - Common aquatic species / livestock drinking";

String class4_use     = "Irrigation";
String class5_use     = "Hazardous Level! No use case!";


void getTSS_NTU(){
 
  float tssV = readChannel(ADS1115_COMP_0_GND); // TODO: coming from ADS1115

  if (tssV > prevMax1) prevMax1 = tssV;
  float mapped_v = _map(tssV, 0.0, prevMax1, 2.5, 4.2); // map to 3.3V to 5V 
  Serial.printf("volt_ori:%f  mapped_v:%f\n", tssV, mapped_v);

  // no need to filter tssV as we are using auto-mapped value 14.04.2022
  // if(tssV < 2.5) tssV = 2.50;
  // if(tssV > 4.2) tssV = 4.200246; // 4.1+/-0.3 equal to NTU<0.5 in pure water


  float tss_ntu = -1120.4*pow(mapped_v,2)+5742.3*mapped_v-4352.9; // formula from https://wiki.dfrobot.com/Turbidity_sensor_SKU__SEN0189
  if(tss_ntu < 0.0) tss_ntu = 0.0;
  if(tss_ntu > 3000.0) tss_ntu = 3000.0; // max NTU readout by Dfrobot

  float tss_mgl = tss_ntu / 3 ; // 1 mgL = 3 NTU

  if(tss_mgl<25)                        classTSS = " [ CLASS I ] ";
  else if(tss_mgl>=25 && tss_mgl<50)    classTSS = " [ CLASS IIA/B ] ";
  else if(tss_mgl>=50 && tss_mgl<150)   classTSS = " [ CLASS III ] ";
  else if(tss_mgl>=150 && tss_mgl<300)  classTSS = " [ CLASS IV ] ";
  else                                  classTSS = " [ CLASS V ] ";

  if(tss_mgl<25){
    // classTSS = " [ CLASS I ] ";
    if(millis()%2 == 0)
      Blynk.virtualWrite(V24, class1_use_ws1);
    else
      Blynk.virtualWrite(V24, class1_use_f1);
  }                        
  else if(tss_mgl>=25 && tss_mgl<50){
    // classTSS = " [ CLASS IIA/B ] ";
    if(millis()%2 == 0)
      Blynk.virtualWrite(V24, class2_use_ws2);
    else
      Blynk.virtualWrite(V24, class2_use_f2);
  }else if(tss_mgl>=50 && tss_mgl<150){
    // classTSS = " [ CLASS III ] ";
    if(millis()%2 == 0)
      Blynk.virtualWrite(V24, class3_use_ws3);
    else
      Blynk.virtualWrite(V24, class3_use_f3);
  }   
  else if(tss_mgl>=150 && tss_mgl<300){
    // classTSS = " [ CLASS IV ] ";
    Blynk.virtualWrite(V24, class4_use);
  }  
  else{
    // classTSS = " [ CLASS V ] ";
    Blynk.virtualWrite(V24, class5_use);
  }                                  
  
  Serial.print("\ttssVolt:");   Serial.print(tssV);
  Serial.print("\ttss_NTU:");   Serial.print(tss_ntu);
  Serial.print("\ttss_mg/L:");  Serial.print(tss_mgl);
  Serial.print("\tClass:");     Serial.println(classTSS);
  Blynk.virtualWrite(V20, tss_ntu);
  Blynk.virtualWrite(V21, tss_mgl);
  Blynk.virtualWrite(V22, classTSS);

  }

float readChannel(ADS1115_MUX channel) {
  float voltage = 0.0;
  adc.setCompareChannels(channel);
  adc.startSingleMeasurement();
  while(adc.isBusy()){}
  voltage = adc.getResult_V(); // alternative: getResult_mV for Millivolt
  return voltage;
}




void blynk_tasks(){
  lcd.clear();
  String dateTime = get_timestamp();
  display_uptime_top_row();
  lcd.setCursor(0,1);
  lcd.print(dateTime);
  Blynk.virtualWrite(V10,uptime_formatter::getUptime());
  Blynk.virtualWrite(V11, dateTime);
  Blynk.virtualWrite(V23, getPH());
  
  Blynk.virtualWrite(V40,busvoltage);
  Blynk.virtualWrite(V41,shuntvoltage);
  Blynk.virtualWrite(V42,current_mA);
}

BLYNK_CONNECTED() {
  timeClient.setTimeOffset(28800);
  restart_ts = get_timestamp();
  Blynk.syncVirtual(V0, V1, V2, V3);
  Blynk.virtualWrite(V12,WiFi.localIP().toString());
  Blynk.virtualWrite(V13,ssid);
  Blynk.virtualWrite(V16, restart_ts); 
  Blynk.virtualWrite(V15,restartCounter);
}

BLYNK_WRITE(V4){
  int pinValue = param.asInt();
  if(pinValue){
    lcd.clear();
    lcd.setCursor(0, 0); // row 0, column 0
    lcd.print("-FORCE RESTART->");
    lcd.setCursor(0, 1); // row 1, column 0
    lcd.print("---REQUESTED!---"); 
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

BLYNK_WRITE(V9){
  int pinValue = param.asInt();
  if(pinValue)  digitalWrite(lcd_backlight,HIGH);
  else          digitalWrite(lcd_backlight,LOW);
}

void BLYNK_HandlerTask(void * pvParameters) 
{
  #ifdef USE_RTC
    timer.setInterval(1000L, printTimeRTC);
  #else
    timer.setInterval(1000L, blynk_tasks);
    timer.setInterval(1000L, getINA219);
    timer.setInterval(1000L, getTSS_NTU);


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
  pinMode(lcd_backlight,OUTPUT);

  digitalWrite(lcd_backlight,HIGH);

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
  if(!adc.init()){
    Serial.println("ADS1115 not connected!");
  }
  adc.setVoltageRange_mV(ADS1115_RANGE_4096); //comment line/change parameter to change range


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

}

void loop() {
  // do not run Blynk here! ESP can't manage it well
}
