#define SMART_EXTENSION

#define BLYNK_PRINT Serial

#ifndef SMART_EXTENSION
#define BLYNK_TEMPLATE_ID "TMPLkJW7Yrd6"
#define BLYNK_DEVICE_NAME "Home Automation"
#define BLYNK_AUTH_TOKEN "zwgdxF7RjqtaRJf8akVfWE4cEXiRFEvd"
#else
#define BLYNK_TEMPLATE_ID "TMPLkJW7Yrd6"
#define BLYNK_DEVICE_NAME "Smart Extension Cord"
#define BLYNK_AUTH_TOKEN "pGraePxIauIZhHislUGs8JCn875bjP6I"
#endif

#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include "uptime_formatter.h"
#include <NTPClient.h>
#include <WiFiUdp.h>
#include "EEPROM.h"


#define EEPROM_SIZE 5
#define restartCounterAddress   0x00 // 15 : 1 byte


#include <Pinger.h>
extern "C"
{
  #include <lwip/icmp.h> // needed for icmp packet definitions
}

char auth[] = BLYNK_AUTH_TOKEN;
#ifdef SMART_EXTENSION
char ssid[] = "UiTM WiFi IoT"; // iPhone 12 Pro Max
char pass[] = "";
#else
char ssid[] = "Maxis Postpaid 128 5G"; // iPhone 12 Pro Max
char pass[] = "respironics";
#endif
const char* remote_host = "blynk.cloud";

#define ONBOARD_LED   2
#define RELAY         0

WidgetLED relayStateLight(V20);
WidgetLED ledStateLight(V21);

Pinger      pinger;
BlynkTimer  timer;
WiFiUDP     ntpUDP;
NTPClient   timeClient(ntpUDP, "pool.ntp.org", 3600, 60000);
String      formattedDate;
String      dayStamp;
String      timeStamp;
bool        automatic = false;
bool        relayAlreadyOn = false;
long        started ;
long        onElapse ;

BLYNK_CONNECTED() {
  Blynk.syncVirtual(V0, V1, V2);
}

void get_ping(){

  // Serial.printf("\n\nPinging Blynk.Cloud\n");
  if(pinger.Ping("blynk.cloud") == false)
  {
    Serial.println("Error during ping command.");
  }

  pinger.OnReceive([](const PingerResponse& response)
  {
    if (response.ReceivedResponse)
    {
      // Serial.printf(
      //   "Reply from %s: bytes=%d time=%lums TTL=%d\n",
      //   response.DestIPAddress.toString().c_str(),
      //   response.EchoMessageSize - sizeof(struct icmp_echo_hdr),
      //   response.ResponseTime,
      //   response.TimeToLive);
    }
    else
    {
      Serial.printf("Request timed out.\n");
    }

    // Return true to continue the ping sequence.
    // If current event returns false, the ping sequence is interrupted.
    return true;
  });
  
  pinger.OnEnd([](const PingerResponse& response)
  {
    // Evaluate lost packet percentage
    float loss = 100;
    if(response.TotalReceivedResponses > 0)
    {
      loss = (response.TotalSentRequests - response.TotalReceivedResponses) * 100 / response.TotalSentRequests;
    }
    
    // Print packet trip data
    // Serial.printf(
    //   "Ping statistics for %s:\n",
    //   response.DestIPAddress.toString().c_str());
    // Serial.printf(
    //   "    Packets: Sent = %lu, Received = %lu, Lost = %lu (%.2f%% loss),\n",
    //   response.TotalSentRequests,
    //   response.TotalReceivedResponses,
    //   response.TotalSentRequests - response.TotalReceivedResponses,
    //   loss);

    // Print time information
    if(response.TotalReceivedResponses > 0)
    {
      // Serial.printf("Approximate round trip times in milli-seconds:\n");
      // Serial.printf(
      //   "    Minimum = %lums, Maximum = %lums, Average = %.2fms\n",
      //   response.MinResponseTime,
      //   response.MaxResponseTime,
      //   response.AvgResponseTime);
      Blynk.virtualWrite(V12, response.AvgResponseTime);

    }
    /*
    // Print host data
  
    Serial.printf("Destination host data:\n");
    Serial.printf(
      "    IP address: %s\n",
      response.DestIPAddress.toString().c_str());
    if(response.DestMacAddress != nullptr)
    {
      Serial.printf(
        "    MAC address: " MACSTR "\n",
        MAC2STR(response.DestMacAddress->addr));
    }
    if(response.DestHostname != "")
    {
      Serial.printf(
        "    DNS name: %s\n",
        response.DestHostname.c_str());
    }
    */

    return true;
  });
  
}

void checkDeviceState(){
  bool relayState = digitalRead(RELAY);
  bool ledState = digitalRead(ONBOARD_LED);

  if(!relayState) {
    relayStateLight.on();
    if(!relayAlreadyOn){
      started = millis();
      relayAlreadyOn = true;
    } 
    onElapse = (millis() - started)/1000;
    String rt = getReadableTime(onElapse);
    Blynk.virtualWrite(V14, rt);
  }
  else {
    relayStateLight.off();
    relayAlreadyOn = false;
    Blynk.virtualWrite(V14, "Relay not activated");
  }

 if(!ledState) ledStateLight.on();
 else ledStateLight.off();
}

void get_uptime(){
  String uptime = String(uptime_formatter::getUptime()); 
  Blynk.virtualWrite(V10, uptime);
}

BLYNK_WRITE(V0)
{
  int pinValue = param.asInt(); 
  if(!automatic){
    digitalWrite(RELAY, !pinValue);   
  }   
}

BLYNK_WRITE(V1)
{
  automatic = param.asInt(); // assigning incoming value from pin V1 to a variable
  digitalWrite(ONBOARD_LED, !automatic);

}

BLYNK_WRITE(V2){
  int pinValue = param.asInt();
  if(pinValue){
    Serial.println("########## CLEARING MEMORY ##########");
    for (int w=0; w < EEPROM_SIZE ; w++){
      EEPROM.write(w, 0);
    }
    EEPROM.commit();
    delay(5);
    Serial.println("########## AUTO RESET ESP ##########");
    ESP.restart();
  }
}

byte check_restart_count(byte prevRC){
  byte newRC = prevRC + 1;  // increment rst count by 1 for each reboot
  EEPROM.write(restartCounterAddress, newRC);
  EEPROM.commit();
  delay(5);

  return newRC;
}


void init_eeprom(){

  EEPROM.begin(EEPROM_SIZE);
  Serial.println(" bytes read from Flash:");
  for (int i = 0; i < EEPROM_SIZE; i++)
  {
    Serial.print("R "); Serial.print(i);   Serial.print(":");
    Serial.print(byte(EEPROM.read(i))); Serial.println();
  }
  Serial.println();
}


void printTimeNTP(){
  timeClient.update();
  formattedDate = timeClient.getFormattedDate();
  // Extract date
  int splitT = formattedDate.indexOf("T");
  dayStamp = formattedDate.substring(0, splitT);
  String dateTime = "Now " + timeClient.getFormattedTime() + "\t\t" + dayStamp;
  Blynk.virtualWrite(V11, dateTime);

}

String getReadableTime(int motion_timeout_sec) {
  String readableTime;
  unsigned long seconds;
  unsigned long minutes;
  unsigned long hours;
  unsigned long days;

  seconds = motion_timeout_sec;
  minutes = seconds / 60;
  hours = minutes / 60;
  days = hours / 24;
  seconds %= 60;
  minutes %= 60;
  hours %= 24;

  if (days > 0) {
    readableTime = String(days) + " ";
  }

  if (hours > 0) {
    readableTime += String(hours) + ":";
  }

  if (minutes < 10) {
    readableTime += "0";
  }
  readableTime += String(minutes) + ":";

  if (seconds < 10) {
    readableTime += "0";
  }
  readableTime += String(seconds);
  return readableTime;
}

void connect_wifi(){
  // WiFi.mode(WIFI_STA); //Optional
    WiFi.begin(ssid);
    Serial.print("\nConnecting to ");
    Serial.println(ssid);

    int i = 0;
    while(WiFi.status() != WL_CONNECTED){
        Serial.print("|");
        if(i % 30 == 0) Serial.println();
        if(i == 10){
          delay(1000);
          ESP.restart();
        }

        on_onboard_led();
        delay(100);
        off_onboard_led();
        delay(100);
        i++;
    }

    Serial.println("\nConnected to the WiFi network");
    Serial.print("Local ESP32 IP: ");
    Serial.println(WiFi.localIP());


}

// the setup function runs once when you press reset or power the board
void setup() {
  // initialize digital pin LED_BUILTIN as an output.
  Serial.begin(115200);

  init_eeprom();
  byte prevRestart = EEPROM.read(restartCounterAddress);
  byte newRestart = check_restart_count(prevRestart);

  pinMode(ONBOARD_LED, OUTPUT);
  pinMode(RELAY, OUTPUT);
  digitalWrite(RELAY, HIGH);   // reset relay

  connect_wifi();
  Blynk.begin(auth, ssid, pass);

  Blynk.virtualWrite(V13,WiFi.localIP().toString());
  Blynk.virtualWrite(V15,ssid);
  Blynk.virtualWrite(V16,newRestart);

  timeClient.begin();
  timeClient.setTimeOffset(28800);

  timer.setInterval(10000L, get_ping);
  timer.setInterval(1000L, checkDeviceState);
  timer.setInterval(1000L, get_uptime);
  timer.setInterval(1000L, printTimeNTP);
}

// the loop function runs over and over again forever
void loop() {
  Blynk.run();
  timer.run();
}

void on_onboard_led(){
    digitalWrite(ONBOARD_LED, HIGH);
}

void off_onboard_led(){
    digitalWrite(ONBOARD_LED, LOW);
}
