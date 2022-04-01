#define BLYNK_TEMPLATE_ID "TMPLUQbmY96z"
#define BLYNK_DEVICE_NAME "ESP8266 ESP01"
#define BLYNK_AUTH_TOKEN "ONaq90OfryTSJbcGYlHlwizh4UHM8zhJ"

#define BLYNK_PRINT Serial
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include "uptime_formatter.h"
#include <NTPClient.h>
#include <WiFiUdp.h>

#include <Pinger.h>
extern "C"
{
  #include <lwip/icmp.h> // needed for icmp packet definitions
}

char auth[] = BLYNK_AUTH_TOKEN;
// char ssid[] = "iPhone 12 Pro Max"; // iPhone 12 Pro Max
// char pass[] = "robotronix";
char ssid[] = "MaxisONE Fibre 2.4G"; // iPhone 12 Pro Max
char pass[] = "respironics";
const char* remote_host = "blynk.cloud";

#define LED_BUILTIN2 2
#define RELAY 0

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
  Blynk.syncVirtual(V0, V1);
}

void get_ping(){

  Serial.printf("\n\nPinging Blynk.Cloud\n");
  if(pinger.Ping("blynk.cloud") == false)
  {
    Serial.println("Error during ping command.");
  }

  pinger.OnReceive([](const PingerResponse& response)
  {
    if (response.ReceivedResponse)
    {
      Serial.printf(
        "Reply from %s: bytes=%d time=%lums TTL=%d\n",
        response.DestIPAddress.toString().c_str(),
        response.EchoMessageSize - sizeof(struct icmp_echo_hdr),
        response.ResponseTime,
        response.TimeToLive);
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
    Serial.printf(
      "Ping statistics for %s:\n",
      response.DestIPAddress.toString().c_str());
    Serial.printf(
      "    Packets: Sent = %lu, Received = %lu, Lost = %lu (%.2f%% loss),\n",
      response.TotalSentRequests,
      response.TotalReceivedResponses,
      response.TotalSentRequests - response.TotalReceivedResponses,
      loss);

    // Print time information
    if(response.TotalReceivedResponses > 0)
    {
      Serial.printf("Approximate round trip times in milli-seconds:\n");
      Serial.printf(
        "    Minimum = %lums, Maximum = %lums, Average = %.2fms\n",
        response.MinResponseTime,
        response.MaxResponseTime,
        response.AvgResponseTime);
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
  bool ledState = digitalRead(LED_BUILTIN2);

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


BLYNK_WRITE(V5)
{
  // You'll get HIGH/1 at startTime and LOW/0 at stopTime.
  // this method will be triggered every day
  // until you remove widget or stop project or
  // clean stop/start fields of widget
  int stateFromTimer = param.asInt(); // assigning incoming value from pin V1 to a variable

  Serial.print("Got a value from Timer event: ");
  Serial.println(stateFromTimer);
  if(automatic){
      if(stateFromTimer) Serial.println("Turning ON Relay and LED");
      else Serial.println("Turning OFF Relay and LED");
      digitalWrite(RELAY, !stateFromTimer);   
      digitalWrite(LED_BUILTIN2, !stateFromTimer);   
  }
}

BLYNK_WRITE(V0)
{
  int pinValue = param.asInt(); 
  if(!automatic){
      Serial.print("Value from manual button: ");
      Serial.println(pinValue);
    digitalWrite(RELAY, !pinValue);   
    digitalWrite(LED_BUILTIN2, !pinValue);
  }   
}

BLYNK_WRITE(V1)
{
  automatic = param.asInt(); // assigning incoming value from pin V1 to a variable
  Serial.print("automation: ");
  Serial.println(automatic);
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

// the setup function runs once when you press reset or power the board
void setup() {
  // initialize digital pin LED_BUILTIN as an output.
  Serial.begin(9600);
  pinMode(LED_BUILTIN2, OUTPUT);
  pinMode(RELAY, OUTPUT);
  digitalWrite(RELAY, LOW);   
  digitalWrite(LED_BUILTIN2, LOW);
  delay(1000);
  
  Blynk.begin(auth, ssid, pass);
  digitalWrite(RELAY, LOW);   
  digitalWrite(LED_BUILTIN2, LOW);
  delay(1000);

   // Ping default gateway
  Serial.printf(
    "\n\nPinging default gateway with IP %s\n",
    WiFi.gatewayIP().toString().c_str());
  if(pinger.Ping(WiFi.gatewayIP()) == false)
  {
    Serial.println("Error during last ping command.");
  }
  delay(5000);

  Blynk.virtualWrite(V13,WiFi.localIP().toString());
  
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
