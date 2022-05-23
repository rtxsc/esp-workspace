#include <esp_now.h>
#include <esp_wifi.h>
#include <WiFi.h>

// Set your Board ID (ESP32 Sender #1 = BOARD_ID 1, ESP32 Sender #2 = BOARD_ID 2, etc)
#define BOARD_ID    0x01  // DO NOT FORGET TO CHANGE THIS ID either 0x01 (client1) or 0x02 (client2)
#define LED         0x02

//MAC Address of the receiver AC:67:B2:25:85:78 (this is the AFS server - THE ONLY ONE SERVER)
uint8_t serverAddress[] = {0xAC, 0x67, 0xB2, 0x25, 0x85, 0x78};

//Structure example to send data
//Must match the receiver structure
typedef struct struct_message {
    int id;
    float temp;
    float humi;
    int randomNum;
    int payload_id;
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

// Insert your SSID following the SSID connected by the server ! TAKE NOTE
constexpr char WIFI_SSID[] = "NPRDC CELCOM M2";

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
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}
 
void setup() {
  //Init Serial Monitor
  Serial.begin(115200);
  pinMode(LED,OUTPUT);

  // Set device as a Wi-Fi Station and set channel
  WiFi.mode(WIFI_STA);

  int32_t channel = getWiFiChannel(WIFI_SSID);

  WiFi.printDiag(Serial); // Uncomment to verify channel number before
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
  esp_wifi_set_promiscuous(false);
  WiFi.printDiag(Serial); // Uncomment to verify channel change after

  //Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
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
 
void loop() {
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    // Save the last time a new reading was published
    previousMillis = currentMillis;
    //Set values to send
    myData.id = BOARD_ID;
    myData.temp = random(20,50);
    myData.humi = random(50,100);
    myData.randomNum = random(0,14);
    myData.payload_id = payload_id++;
     
    //Send message via ESP-NOW
    esp_err_t result = esp_now_send(serverAddress, (uint8_t *) &myData, sizeof(myData));
    if (result == ESP_OK) {
      Serial.println("Sent with success");
    }
    else {
      Serial.println("Error sending the data");
    }
  }
}