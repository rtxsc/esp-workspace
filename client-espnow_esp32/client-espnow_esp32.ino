#include <esp_now.h>
#include <esp_wifi.h>
#include <WiFi.h>
#include <Adafruit_INA219.h>

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// #define LOCATION_MKE2_UiTM_WiFi_IoT // comment this line for TBS deployment
// #define LOCATION_MKE2_MaxisONE // comment this line for TBS deployment

Adafruit_INA219     ina219_A;

bool GROVE_LCD_AVAILABLE = false; // true for debugging purpose 27.02.2023 | change to false 03.03.2023
bool INA219_AVAILABLE = false;
bool ADS1115_AVAILABLE = false;
bool OLED_AVAILABLE = false;

float shuntvoltage;
float busvoltage;
float current_mA;
String busvoltage_str;
String current_mA_str;

// #define ESP32C3
#define ESP32S2mini

/*
ESP32C3-6 & ESP32C3-8 at TBS linking with DEV_5 (AASAS M10)
ESP32C3-4 & ESP32C3-7 at MKE2 linking with S2_3 (AASAS M03)
ESP32C3-3 is currently a work in progress (container & booster upgrade)
ESP32S2m1 48:27:e2:5d:c5:1c / 48:27:E2:5D:C5:1C
*/

// Set your Board ID (ESP32 Sender #1 = BOARD_ID 1, ESP32 Sender #2 = BOARD_ID 2, etc)
#ifdef ESP32C3
  #define BOARD_ID    "ESP32C3-1" // enter model name here ESPC3-3 / -4 / -5 / -6 / -7 / -8 
  #define LED         19    // 19 if ESP32-C3
  #define RED         0x03 // 0x03 if ESP32-C3
  #define GRN         0x04 // 0x04 if ESP32-C3
  #define BLU         0x05 // 0x05 if ESP32-C3
#elif defined ESP32S2mini
  #define BOARD_ID    "ESP32S2m1" // enter model name here ESP32S2m1 / -m2 / -m3
  #define LED         15    
  #define RED         0x03 
  #define GRN         0x04 
  #define BLU         0x05 
#else
  #define BOARD_ID    "Unknown"  // DO NOT FORGET TO CHANGE THIS ID either 0x01 (client1) or 0x02 (client2)
  #define LED         0x02  // 0x02 if ESP01/ESP32 
  #define RED         23    // 23 NOT USED (JUST DUMMY)
  #define GRN         18 
  #define BLU         19 
#endif

bool send_success = false;
//MAC Address of the receiver AC:67:B2:25:85:78 (this is the AFS server - THE ONLY ONE SERVER)
//uint8_t serverAddress[] = {0xAC, 0x67, 0xB2, 0x25, 0x85, 0x78}; // AFS(one) at Block N
// uint8_t serverAddress[] = {0xC8, 0x2B, 0x96, 0xB9, 0xA9, 0x58}; // AFS2 at MKE2
// Insert your SSID following the SSID connected by the server ! TAKE NOTE

#ifdef LOCATION_MKE2_UiTM_WiFi_IoT
  const char WIFI_SSID[] = "UiTM WiFi IoT"; //. constexpr
  uint8_t serverAddress[] = {0x7C, 0xDF, 0xA1, 0x00, 0xBA, 0x9E}; // ESP32S2-3
  const char serverName[] = "ESP32S2-3";
#elif defined LOCATION_MKE2_MaxisONE
  const char WIFI_SSID[] = "MaxisONE Fibre 2.4G";
  uint8_t serverAddress[] = {0x7C, 0xDF, 0xA1, 0x00, 0xBA, 0x9E}; // ESP32S2-3
  const char serverName[] = "ESP32S2-3";
#else
  const char WIFI_SSID[] = "MaxisONE Fibre 2.4G_EXT";
  uint8_t serverAddress[] = {0x84, 0x0D, 0x8E, 0xE2, 0xD6, 0xD8}; // ESP32DEV_5 (TBS) 
  const char serverName[] = "ESP32DEV_5/AASAS M10";
#endif

//Structure example to send data
//Must EXACTLY MATCH THE PATTERN of the receiver's structure
typedef struct struct_message {
    String id;
    unsigned int payload_id;
    float volt;
    float amps;
    float temp;
    float humi;
    float mois;
    float rain;
} struct_message;

//Create a struct_message called myData
struct_message myData;

// Structure example to receive data (control) signal sent by the server via Blynk
// Must match the sender structure (sender is AFS server)
typedef struct struct_control {
    int control;
} struct_control;

// Create a struct_message called recvControl (received from server AFS AC:67:B2:25:85:78) - 25.03.2022
struct_control recvControl;

unsigned long previousMillis = 0;   // Stores last time temperature was published
const int interval = 5000;        // Interval at which to publish sensor readings 5 seconds for stability

unsigned int payload_id = 0;


int32_t getWiFiChannel(const char *ssid) {
  if (int32_t n = WiFi.scanNetworks()) {
      for (uint8_t i=0; i<n; i++) {
          if (!strcmp(ssid, WiFi.SSID(i).c_str())) {
              return WiFi.channel(i);
          }
      }
  }
  return 0;
}


// callback when data is sent
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
 Serial.print("\n\nBoard ID:");
//  Serial.println(BOARD_ID,HEX);
  Serial.println(BOARD_ID);

  Serial.print("Last Packet Send Status:");
  if(status == ESP_NOW_SEND_SUCCESS)  send_success = true;
  else                                send_success = false;
  // ESP_NOW_SEND_SUCCESS = "Delivery Success"  ||   ESP_NOW_SEND_FAIL = "Delivery Fail"
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "[Delivery Success]" : "[Delivery Fail]");
}

void initINA219(){
  if (! ina219_A.begin()) {
    Serial.println("Failed to find INA219_A chip");
    while (1) { delay(10); }
  }
  ina219_A.setCalibration_32V_2A();
}

void getINA219(){
  shuntvoltage = ina219_A.getShuntVoltage_mV(); // measured across Vin+ & Vin-
  busvoltage = ina219_A.getBusVoltage_V(); // measured at Vin+ only
  current_mA = ina219_A.getCurrent_mA();
  busvoltage_str = String(busvoltage,2);
  current_mA_str = String(current_mA,2);
  Serial.printf("V:%.2fV I:%.2fmA\n", busvoltage, current_mA);
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
      if(address == 0x3C) OLED_AVAILABLE      = true;
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

void setup() {
  //Init Serial Monitor
  Serial.begin(115200);
     
  Wire.begin();
  i2c_scan();

  if(INA219_AVAILABLE){
    initINA219();
  }
  pinMode(LED,OUTPUT);
  pinMode(RED,OUTPUT);
  pinMode(GRN,OUTPUT);
  pinMode(BLU,OUTPUT);

for(int i=0;i<3;i++){
  digitalWrite(RED,HIGH);
  digitalWrite(BLU,0);
  digitalWrite(GRN,0);
  delay(200);
  digitalWrite(RED,0);
  digitalWrite(BLU,HIGH);
  digitalWrite(GRN,0);
  delay(200);
  digitalWrite(RED,0);
  digitalWrite(BLU,0);
  digitalWrite(GRN,HIGH);
  delay(200);
}  

  String mac_str = WiFi.macAddress();
  const char* mac_addr = mac_str.c_str();
  Serial.print("MAC (String):");
  Serial.println(mac_str);
  Serial.printf("MAC (const char): %s\n", mac_addr);
  // WiFi.disconnect();
  // WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE, INADDR_NONE);


  #ifdef ESP32S2mini

  if(OLED_AVAILABLE){
    // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
    if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
      Serial.println(F("SSD1306 allocation failed"));
      for(;;); // Don't proceed, loop forever
    }    

    display.display();
    delay(1000); // Pause for 2 seconds

    // Clear the buffer
    display.clearDisplay();
    display.setTextSize(1);             // Normal 1:1 pixel scale
    display.setTextColor(SSD1306_WHITE);        // Draw white text
    display.setCursor(0,0);             // Start at top-left corner
    display.println("ESPNOW CLIENT\n MON 26/06/2023");
    display.setCursor(0,16);             // Start at top-left corner

    display.setTextColor(SSD1306_BLACK, SSD1306_WHITE); // Draw 'inverse' text
    display.println("Board ID: " + String(BOARD_ID));

    display.setCursor(0,32);             // Start at top-left corner
    display.setTextSize(1);             // Draw 2X-scale text
    display.setTextColor(SSD1306_BLACK, SSD1306_WHITE); // Draw 'inverse' text
    display.println(mac_str);
    display.display();


    display.setCursor(0,48);             // Start at top-left corner
    display.setTextColor(SSD1306_WHITE); // Draw 'inverse' text
    for(int i=0; i < 42 ; i++){
      display.print('|');
      display.display();
      delay(10);
    }
  }
 
#endif  

  // Set device as a Wi-Fi Station and set channel
  WiFi.mode(WIFI_STA); // WIFI_AP WIFI_AP_STA WIFI_STA
  Serial.print("WiFi SSID that this ESP is connecting to explicitly: ");
  Serial.println(WIFI_SSID);

  int32_t channel = getWiFiChannel(WIFI_SSID);

  WiFi.printDiag(Serial); // Uncomment to verify channel number before
  esp_wifi_set_promiscuous(1); // default true
  esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
  esp_wifi_set_promiscuous(0); // default false
  WiFi.printDiag(Serial); // Uncomment to verify channel change after

  //Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    digitalWrite(LED,LOW);
    return;
  }else{
    digitalWrite(LED,HIGH);    
  }
    // Once ESPNow is successfully Init, we will register for recv CB to
  // get recv packer info
  esp_now_register_recv_cb(OnDataRecv);

  // Once ESPNow is successfully Init, we will register for Send CB to
  // get the status of Trasnmitted packet
  esp_now_register_send_cb(OnDataSent);
  
  // Register peer (to register with server as the only ONE peer)
  esp_now_peer_info_t peerInfo;
  memset(&peerInfo, 0, sizeof(peerInfo)); // https://github.com/espressif/arduino-esp32/issues/6029
  memcpy(peerInfo.peer_addr, serverAddress, 6);
  peerInfo.encrypt = false;
  
  //Add peer        
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add peer");
    return;
  }
  digitalWrite(LED, LOW); // reset the LED to OFF initially (not using the state of pin at the moment 25.03.2022)
  digitalWrite(RED, LOW); // reset the LED to OFF initially (not using the state of pin at the moment 25.03.2022)
  digitalWrite(GRN, LOW); // reset the LED to OFF initially (not using the state of pin at the moment 25.03.2022)
  digitalWrite(BLU, LOW); // reset the LED to OFF initially (not using the state of pin at the moment 25.03.2022)

}

// callback function that will be executed when data is received
void OnDataRecv(const uint8_t * mac_addr, const uint8_t *incomingData, int len) { 
  // Copies the sender mac address to a string
  char macStr[18];
  Serial.print("Packet received from: ");
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
           mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  Serial.println(macStr);
  memcpy(&recvControl, incomingData, sizeof(recvControl));

  int recvValue = recvControl.control;
  Serial.print("Received control signal:");
  Serial.println(recvValue);
  if(recvValue == 1)      digitalWrite(LED,HIGH);
  else                    digitalWrite(LED,LOW);

}
 
bool timeout = false;

void loop() {
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    timeout = false;    
    if(INA219_AVAILABLE){
      getINA219();
    }else{
      busvoltage = -1.0;
      current_mA = -1.0;
    }
    // Save the last time a new reading was published
    previousMillis = currentMillis;
    //Set values to send
    myData.id = BOARD_ID;
    myData.payload_id = payload_id++;
    myData.volt = busvoltage;
    myData.amps = current_mA;
    myData.temp = random(20,50);
    myData.humi = analogRead(2); // random(50,100);
    myData.mois = map(analogRead(3),0,2500,0,100); // moist percent
    myData.rain = analogRead(1); // IO1

    //Send message via ESP-NOW
    esp_err_t result = esp_now_send(serverAddress, (uint8_t *) &myData, sizeof(myData));
    Serial.print("Result (esp_err_t):"); Serial.println(result); 

    if (result == ESP_OK && send_success) {
      Serial.println("[SHOULD BLINK BLUE] Sent with success");
      digitalWrite(BLU, HIGH); 
      digitalWrite(LED, HIGH);
      delay(50);
    }
    else {
      Serial.println("[SHOULD BLINK RED] Error sending the data");
      digitalWrite(GRN, HIGH);
      delay(50);
    }

      String res;
      if(send_success) 
        res = "Success";
      else
        res = "Failed";

      if(OLED_AVAILABLE){
        display.clearDisplay();
        display.setTextSize(1);             // Normal 1:1 pixel scale
        display.setTextColor(SSD1306_WHITE);        // Draw white text
        display.setCursor(0,0);             // Start at top-left corner
        display.println("P:"+String(payload_id));
        display.setCursor(64,0); 
        display.setTextColor(SSD1306_BLACK, SSD1306_WHITE); // Draw 'inverse' text
        display.println("Res:"+ res);
        display.setCursor(0,8);             // Start at top-left corner
        display.setTextSize(1);             // Draw 2X-scale text
        display.setTextColor(SSD1306_WHITE);
        display.println("V:"+String(busvoltage)+"V I:"+String(current_mA)+"mA");
        display.setCursor(0,16);             // Start at top-left corner
        display.setTextSize(1);             // Draw 2X-scale text
        display.setTextColor(SSD1306_WHITE);
        display.println("T:"+String(myData.temp)+" H:"+String(myData.humi));
        display.setCursor(0,24);             // Start at top-left corner
        display.setTextSize(1);             // Draw 2X-scale text
        display.setTextColor(SSD1306_WHITE);
        display.println("M:"+String(myData.mois)+" R:"+String(myData.rain));
        display.setCursor(0,40);             // Start at top-left corner
        display.setTextSize(1);             // Draw 2X-scale text
        display.setTextColor(SSD1306_BLACK, SSD1306_WHITE); // Draw 'inverse' text
        display.println("ID: "+ String(BOARD_ID));
        display.setCursor(0,50);             // Start at top-left corner
        display.setTextSize(1);             // Draw 2X-scale text
        display.setTextColor(SSD1306_BLACK, SSD1306_WHITE); // Draw 'inverse' text
        display.println("HS: "+ String(serverName)); // handshake server
        display.display();

      }
  } // end of if
  else{
    if(!timeout){
      timeout = true;
      display.setCursor(0,32);             // Start at top-left corner
      display.setTextSize(1);             // Draw 2X-scale text
      display.setTextColor(SSD1306_WHITE);
    }
    if(currentMillis - previousMillis < interval){
      // currentMillis = millis();
      display.print("/");
      display.display();
      delay(200);
    }
  }

  digitalWrite(LED, LOW);
  digitalWrite(BLU, LOW);
  digitalWrite(RED, LOW); 
  digitalWrite(GRN, LOW); 

}
