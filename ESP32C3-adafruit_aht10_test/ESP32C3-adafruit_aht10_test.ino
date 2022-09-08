#include <Adafruit_AHT10.h>
#include <Wire.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include "uptime_formatter.h"
#define RED     3
#define GRN     4
#define BLU     5
#define I2C_SDA 8 // 0 for ESP01s
#define I2C_SCL 9 // 2 for ESP01s

#define CLK 6
#define DIO 7
#define LED18 18
#define LED19 19

Adafruit_AHT10 aht;

#include "GyverTM1637.h"
GyverTM1637 disp(CLK, DIO);

// setting PWM properties
const int freq = 5000;
const int ledChannelR = 0;
const int ledChannelG = 1;
const int ledChannelB = 2;

const int resolution = 12; // Resolution 8, 10, 12, 15
int max_adc = pow(2,resolution)-1;
int prev_tempc;


String NODE_NUMBER = "ESP1" ;
#define ISDestURL "insecure-groker.initialstate.com" // https can't be handled by the ESP8266, thus "insecure"
#define bucketKey "UXVB94NLT8CT" // Bucket key (hidden reference to your bucket that allows appending):
#define bucketName "ESP32C3-XBOX-SERIES-X" // Bucket name (name your data will be associated with in Initial State):
#define accessKey "ist_jXh1F13mOQDwsBRQdcMHjvvlAVyLbTRi"


#define SUCCESS                 1
#define UNSUCCESSFUL            0
bool payload_pushed = false;
long payload_push_interval;
int IS_PUSH_INTERVAL = 3600000; // default to 1 hour

String signalName[] = { 
                          "temperature_",
                          "humidity_",
                          "Uptime_"
                      };

const byte payloadSize = sizeof(signalName)/sizeof(signalName[0]);
String signalData[payloadSize];
char ssid[] = "MaxisONE Fibre 2.4G";
char pass[] = "respironics";
WiFiClient       clientIS; // 13.06.2022 Monday (with teammate)

void setup() {
  Serial.begin(115200);
  Wire.begin(I2C_SDA, I2C_SCL);
  disp.clear();
  disp.brightness(4);
  pinMode(RED, OUTPUT);
  pinMode(GRN, OUTPUT);
  pinMode(BLU, OUTPUT);
  pinMode(LED18, OUTPUT);
  pinMode(LED19, OUTPUT);


    // configure LED PWM 
  ledcSetup(ledChannelR, freq, resolution);
  ledcSetup(ledChannelG, freq, resolution);
  ledcSetup(ledChannelB, freq, resolution);

  // attach the channel to the GPIO to be controlled
  ledcAttachPin(RED, ledChannelR);
  ledcAttachPin(GRN, ledChannelG);
  ledcAttachPin(BLU, ledChannelB);

  Serial.println("Adafruit AHT10 demo!");

  if (! aht.begin()) {
    Serial.println("Could not find AHT10? Check wiring");
    while (1) delay(10);
  }
  Serial.println("AHT10 found");
  digitalWrite(RED,1);
  digitalWrite(GRN,0);
  digitalWrite(BLU,0);
  delay(500);
  digitalWrite(RED,0);
  digitalWrite(GRN,1);
  digitalWrite(BLU,0);
  delay(500);
  digitalWrite(RED,0);
  digitalWrite(GRN,0);
  digitalWrite(BLU,1);
  delay(500);
  digitalWrite(RED,0);
  digitalWrite(GRN,0);
  digitalWrite(BLU,0);
  delay(500);

  digitalWrite(LED18,0);
  digitalWrite(LED19,0);


}

void loop() {
  sensors_event_t humidity, temp;
  aht.getEvent(&humidity, &temp);// populate temp and humidity objects with fresh data
  int tempc = temp.temperature;
  int humid = humidity.relative_humidity;
  if (tempc == prev_tempc)
  {
    max_adc = 256;
    disp.brightness(2);
  }
  else{
    max_adc = pow(2,resolution)-1;
    disp.brightness(7);
  }
  // // DUMMY SECTION STARTS
  // float dummy_tempc = analogRead(0);
  // dummy_tempc = map(dummy_tempc,0,4095,15,60);
  // tempc = dummy_tempc;
  // // DUMMY SECTION ENDS

  // Serial.print("Temperature: "); Serial.print(tempc); Serial.println(" degrees C");
  // Serial.print("Humidity: "); Serial.print(humid); Serial.println("% rH");

  map_rgb(tempc);

  disp.point(1);   // выкл точки
  disp.displayClockTwist(tempc,humid,50);    // выводим время
  prev_tempc = tempc; // save current temp as previous

  delay(100);

}

void map_rgb(float temp){
    float grn_val = 0.0;
    float blu_val = 0.0;
    float red_val = 0.0;

    if(temp < 30){
      // TOTALLY BLUE AREA
      ledcWrite(ledChannelR,0);
      ledcWrite(ledChannelG,0);
      ledcWrite(ledChannelB,max_adc);
    }
    else if(temp >= 30 && temp < 50){
      // BLUE TO GREEN AREA
      blu_val = map(temp, 20, 34.9, max_adc, 0);  // blue --
      blu_val = constrain(blu_val, 0,max_adc);
      grn_val = max_adc-blu_val;                  // green ++
      ledcWrite(ledChannelR,0);
      ledcWrite(ledChannelG,grn_val);
      ledcWrite(ledChannelB,blu_val);
    }
    else if(temp == 50){
      // TOTALLY GREEN AREA
      ledcWrite(ledChannelR,0);
      ledcWrite(ledChannelG,max_adc);
      ledcWrite(ledChannelB,0);
    }
    else if(temp > 50  && temp < 55){ 
      // GREEN TO RED AREA
      red_val = map(temp, 35, 49.9, 0, max_adc); // red ++
      red_val = constrain(red_val, 0, max_adc);
      grn_val = max_adc-red_val;                 // green --
      ledcWrite(ledChannelR,red_val);
      ledcWrite(ledChannelG,grn_val);
      ledcWrite(ledChannelB,0);
    }
    else{
      // exceeding 55 degree celcius
      // TOTALLY RED AREA
      ledcWrite(ledChannelR,max_adc);
      ledcWrite(ledChannelG,0);
      ledcWrite(ledChannelB,0);
    }


}



// this method makes a HTTP connection to the server and creates a bucket is it does not exist:
bool postBucket() {
// close any connection before send a new request.
// This will free the socket on the WiFi shield
clientIS.stop();

// if there's a successful connection:
if (clientIS.connect(ISDestURL, 80) == SUCCESS) {
 Serial.println("connecting...");
  // send the HTTP PUT request:
  // Build HTTP request.
  String toSend = "POST /api/buckets HTTP/1.1\r\n";
  toSend += "Host:";
  toSend += ISDestURL;
  toSend += "\r\n" ;
  toSend += "User-Agent:Arduino\r\n";
  toSend += "Accept-Version: ~0\r\n";
  toSend += "X-IS-AccessKey: " accessKey "\r\n";
  toSend += "Content-Type: application/json\r\n";
  String payload = "{\"bucketKey\": \"" bucketKey "\",";
  payload += "\"bucketName\": \"" bucketName "\"}";
  payload += "\r\n";
  toSend += "Content-Length: "+String(payload.length())+"\r\n";
  toSend += "\r\n";
  toSend += payload;

  clientIS.println(toSend);
  // Serial.println(toSend);
  return true;
} else {
  // if you couldn't make a connection:
  clientIS.stop();
  vTaskDelay(1000 / portTICK_PERIOD_MS);
  return false;
  }
}


void static_postData() {
  // close any connection before send a new request.
  // This will free the socket on the WiFi shield
  clientIS.stop();
  // if there's a successful connection:
  Serial.println("[static_postData] clientIS stopped and contacting ISDestURL ...");

  if (clientIS.connect(ISDestURL, 80) == SUCCESS) {
   Serial.println("[static_postData] connection to ISDestURL successful...");
    // send the HTTP PUT request:
    // Build HTTP request.

    for (int i=0; i<payloadSize; i++){
      String toSend = "POST /api/events HTTP/1.1\r\n";
      toSend += "Host:";
      toSend += ISDestURL;
      toSend += "\r\n" ;
      toSend += "Content-Type: application/json\r\n";
      toSend += "User-Agent: Arduino\r\n";
      toSend += "Accept-Version: ~0\r\n";
      toSend += "X-IS-AccessKey: " accessKey "\r\n";
      toSend += "X-IS-BucketKey: " bucketKey "\r\n";

      String payload = "[{\"key\": \"" + signalName[i] + "\", ";
      payload +="\"value\": \"" + signalData[i] + "\"}]\r\n";

      toSend += "Content-Length: "+String(payload.length())+"\r\n";
      toSend += "\r\n";
      toSend += payload;
      // Serial.println(payload);
      // Serial.println(toSend);
      clientIS.println(toSend);
    }
    payload_pushed = true;
  }
} 


void handle_posting(int t, int h){
  
  byte payloadCount = 0;

  signalData[payloadCount] = String(t);              
  signalName[payloadCount] = "temperature_"+NODE_NUMBER;
  payloadCount++;

  signalData[payloadCount] = String(h);       
  signalName[payloadCount] = "humidity_"+NODE_NUMBER;
  payloadCount++;

  signalData[payloadCount] = String(uptime_formatter::getUptime()); 
  signalName[payloadCount] = "Uptime_"+NODE_NUMBER;
  payloadCount++;

  
  while(!payload_pushed && millis() - payload_push_interval > IS_PUSH_INTERVAL){

    // for(int i=0; i < payloadCount ; i++){
    //   Serial.print("Configuring signalName ["); Serial.print(i); Serial.print("] "); 
    //   Serial.print(signalName[i]);
    //   Serial.print(" with latest data of "); Serial.println(signalData[i]);
    // }
   
    Serial.print("\n\nPushing static data at every ");
    Serial.println(IS_PUSH_INTERVAL);
  
    static_postData();

    payload_push_interval = millis();
    Serial.println("Pushing static data DONE");

  }
  
  
  
  
  }
