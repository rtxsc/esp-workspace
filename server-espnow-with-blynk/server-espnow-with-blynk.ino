#define BLYNK_TEMPLATE_ID "TMPLLcUZS8pw"
#define BLYNK_DEVICE_NAME "AFS"
#define BLYNK_AUTH_TOKEN "3G4XbLzWHurLKwzeAeKQZH7QttvcM9gR"
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

TaskHandle_t Task1;
AsyncWebServer server(80);
AsyncEventSource events("/events");


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

// Replace with your network credentials (STATION)
char auth[] = BLYNK_AUTH_TOKEN;
const char* ssid = "NPRDC CELCOM M2";
const char* pass = "nprdc1234";
const char* remote_host = "blynk.cloud";

int board_id = 0;

String jsonString1 = "None";
String dt1 = "None";
float temp1 = 0;
float humi1 = 0;
int read_id1 = 0;

String jsonString2 = "None";
String dt2 = "None";
float temp2 = 0;
float humi2 = 0;
int read_id2 = 0;

// Structure example to receive data
// Must match the sender structure
typedef struct struct_message {
  int id;
  float temp;
  float hum;
  unsigned int readingId;
} struct_message;

struct_message incomingReadings;

JSONVar     board;
BlynkTimer  timer;
DS1307      _clock;
rgb_lcd     lcd;
WiFiUDP     ntpUDP;
NTPClient   timeClient(ntpUDP, "pool.ntp.org", 3600, 60000);

// Variables to save date and time
String formattedDate;
String dayStamp;
String timeStamp;


const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <title>ESP-NOW DASHBOARD</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <link rel="stylesheet" href="https://use.fontawesome.com/releases/v5.7.2/css/all.css" integrity="sha384-fnmOCqbTlWIlj8LyTjo7mOUStjsKC4pOpQbqyi7RrhN7udi9RwhKkMHpvLbHG9Sr" crossorigin="anonymous">
  <link rel="icon" href="data:,">
  <style>
    html {font-family: Arial; display: inline-block; text-align: center;}
    p {  font-size: 1.2rem;}
    body {  margin: 0;}
    .topnav { overflow: hidden; background-color: #2f4468; color: white; font-size: 1.7rem; }
    .content { padding: 20px; }
    .card { background-color: white; box-shadow: 2px 2px 12px 1px rgba(140,140,140,.5); }
    .cards { max-width: 700px; margin: 0 auto; display: grid; grid-gap: 2rem; grid-template-columns: repeat(auto-fit, minmax(300px, 1fr)); }
    .reading { font-size: 2.8rem; }
    .packet { color: #bebebe; }
    .card.temperature { color: #fd7e14; }
    .card.humidity { color: #1b78e2; }
  </style>
</head>
<body>
  <div class="topnav">
    <h3>ESP-NOW DASHBOARD</h3>
  </div>
  <div class="content">
    <div class="cards">
      <div class="card temperature">
        <h4><i class="fas fa-thermometer-half"></i> BOARD #1 - TEMPERATURE</h4><p><span class="reading"><span id="t1"></span> &deg;C</span></p><p class="packet">Reading ID: <span id="rt1"></span></p>
      </div>
      <div class="card humidity">
        <h4><i class="fas fa-tint"></i> BOARD #1 - HUMIDITY</h4><p><span class="reading"><span id="h1"></span> &percnt;</span></p><p class="packet">Reading ID: <span id="rh1"></span></p>
      </div>
      <div class="card temperature">
        <h4><i class="fas fa-thermometer-half"></i> BOARD #2 - TEMPERATURE</h4><p><span class="reading"><span id="t2"></span> &deg;C</span></p><p class="packet">Reading ID: <span id="rt2"></span></p>
      </div>
      <div class="card humidity">
        <h4><i class="fas fa-tint"></i> BOARD #2 - HUMIDITY</h4><p><span class="reading"><span id="h2"></span> &percnt;</span></p><p class="packet">Reading ID: <span id="rh2"></span></p>
      </div>
    </div>
  </div>
<script>
if (!!window.EventSource) {
 var source = new EventSource('/events');
 
 source.addEventListener('open', function(e) {
  console.log("Events Connected");
 }, false);
 source.addEventListener('error', function(e) {
  if (e.target.readyState != EventSource.OPEN) {
    console.log("Events Disconnected");
  }
 }, false);
 
 source.addEventListener('message', function(e) {
  console.log("message", e.data);
 }, false);
 
 source.addEventListener('new_readings', function(e) {
  console.log("new_readings", e.data);
  var obj = JSON.parse(e.data);
  document.getElementById("t"+obj.id).innerHTML = obj.temperature.toFixed(2);
  document.getElementById("h"+obj.id).innerHTML = obj.humidity.toFixed(2);
  document.getElementById("rt"+obj.id).innerHTML = obj.readingId;
  document.getElementById("rh"+obj.id).innerHTML = obj.readingId;
 }, false);
}
</script>
</body>
</html>)rawliteral";


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


void myTimerEvent()
{
  Blynk.virtualWrite(V10, millis() / 1000);
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
  Blynk.virtualWrite(V13,jsonString1 + " | Received at: " + dt1);
  Blynk.virtualWrite(V20,temp1);
  Blynk.virtualWrite(V21,humi1);
  Blynk.virtualWrite(V22,read_id1);

  Blynk.virtualWrite(V14,jsonString2 + " | Received at: " + dt2);
  Blynk.virtualWrite(V24,temp2);
  Blynk.virtualWrite(V25,humi2);
  Blynk.virtualWrite(V26,read_id2);

}

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

// callback function that will be executed when data is received
void OnDataRecv(const uint8_t * mac_addr, const uint8_t *incomingData, int len) { 
  // Copies the sender mac address to a string
  char macStr[18];
  Serial.print("Packet received from: ");
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
           mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  Serial.println(macStr);
  memcpy(&incomingReadings, incomingData, sizeof(incomingReadings));

  board["id"] = incomingReadings.id;
  board["temperature"] = incomingReadings.temp;
  board["humidity"] = incomingReadings.hum;
  board["readingId"] = String(incomingReadings.readingId);


  if(incomingReadings.id == 1){
    temp1 = incomingReadings.temp;
    humi1 = incomingReadings.hum;
    read_id1 = incomingReadings.readingId;
    dt1 = get_timestamp();
    jsonString1 = JSON.stringify(board);
    // events.send(jsonString.c_str(), "new_readings", millis());
    Serial.printf("Board ID %u: %u bytes\n", incomingReadings.id, len);
    Serial.printf("temp value: %4.2f \n", temp1);
    Serial.printf("humi value: %4.2f \n", humi1);
    Serial.printf("readingID value: %d \n", read_id1);
    Serial.println();
  }
  else if(incomingReadings.id == 2){
    temp2 = incomingReadings.temp;
    humi2 = incomingReadings.hum;
    read_id2 = incomingReadings.readingId;
    dt2 = get_timestamp();
    jsonString2 = JSON.stringify(board);
    // events.send(jsonString2.c_str(), "new_readings", millis());
    Serial.printf("2-Board ID %u: %u bytes\n", incomingReadings.id, len);
    Serial.printf("2-temp value: %4.2f \n", temp2);
    Serial.printf("2-humi value: %4.2f \n", humi2);
    Serial.printf("2-readingID value: %d \n", read_id2);
    Serial.println();
  }
  else{
    Serial.println("No payload received");
  }
  
}

void ESPNOW_HandlerTask(void * pvParameters) // previously void loop()
{
  Serial.print("ESPNOW_HandlerTask running on core ");
  Serial.println(xPortGetCoreID());
  // Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  // Once ESPNow is successfully Init, we will register for recv CB to
  // get recv packer info
  esp_now_register_recv_cb(OnDataRecv);

  /* DISABLING LOCAL SERVER IN FAVOR OF BLYNK OPERATIONS FREEING UP MORE CPU RESOURCES 24.03.2022*/
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html);
  });
   
  events.onConnect([](AsyncEventSourceClient *client){
    if(client->lastId()){
      Serial.printf("Client reconnected! Last message ID that it got is: %u\n", client->lastId());
    }
    // send event with message "hello!", id current millis
    // and set reconnect delay to 1 second
    client->send("hello!", NULL, millis(), 10000);
  });
  server.addHandler(&events);
  server.begin();
  for(;;){    
      static unsigned long lastEventTime = millis();
      static const unsigned long EVENT_INTERVAL_MS = 5000;
      if ((millis() - lastEventTime) > EVENT_INTERVAL_MS) {
        events.send("ping",NULL,millis());
        lastEventTime = millis();
    }
  }
} // end of FreeRTOS handler

void setup() {
  // Initialize Serial Monitor
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
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("!WiFi Connected!");
  lcd.setCursor(0,1);
  lcd.print("-Using NTP Time-");
  delay(1000);

  timeClient.begin();
  timeClient.setTimeOffset(28800);

  #ifdef USE_RTC
    _clock.begin();
    _clock.fillByYMD(2021,12,24);
    _clock.fillByHMS(12,30,30);
    _clock.fillDayOfWeek(FRI);
    _clock.setTime();
  #endif

  Blynk.virtualWrite(V12,WiFi.localIP().toString());
  Serial.println(WiFi.macAddress()); 

  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Connecting Blynk");
  lcd.setCursor(0,1);
  lcd.print("blynk.cloud:80");

  Serial.print("Station IP Address: ");
  Serial.println(WiFi.localIP());
  Serial.print("Wi-Fi Channel: ");
  Serial.println(WiFi.channel());
  Serial.print("ESP Board MAC Address:  ");
  Serial.println(WiFi.macAddress());


  Blynk.begin(auth, ssid, pass);
  Serial.print("[BLYNK WiFi Handler] WiFi connected with IP ");  
  Serial.println(WiFi.localIP());
  // timer.setInterval(100, handle_server_event);
  // timer.setInterval(15000L, Get_Ping);
  #ifdef USE_RTC
    timer.setInterval(1000L, printTimeRTC);
  #else
    timer.setInterval(1000L, printTimeNTP);
  #endif

  xTaskCreatePinnedToCore(
                    ESPNOW_HandlerTask,       /* Task function. */
                    "ESPNOW_HandlerTask",     /* name of task. */
                    10000,                    /* Stack size of task */
                    NULL,                     /* parameter of the task */
                    1,                        /* priority of the task */
                    &Task1,                   /* Task handle to keep track of created task */
                    CORE_1);                  /* pin task to core 0 */                  
  delay(500); 
}

void loop() {
  Blynk.run();
  timer.run();
}

