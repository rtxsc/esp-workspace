
#include <BlynkSimpleEsp8266.h>
#include <ESP8266WiFi.h>
#include "uptime_formatter.h"
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <PubSubClient.h>

#define ssid_0 "NPRDC CELCOM M1"
#define ssid_1 "NPRDC CELCOM M2"

#include <Pinger.h>
extern "C"
{
  #include <lwip/icmp.h> // needed for icmp packet definitions
}

const char* mqtt_server = "broker.emqx.io"; // "broker.mqtt-dashboard.com"; 
const int   mqtt_port = 1883; // 1883;

char* ssid = ssid_0;
const char* pass = "nprdc1234";
//char ssid[] = "iPhone 12 Pro Max"; // iPhone 12 Pro Max
//char pass[] = "robotronix";
//char ssid[] = "MaxisONE Fibre 2.4G"; // iPhone 12 Pro Max
//char pass[] = "respironics";

#define RELAY   0
#define LED02   5
#define MOTION  2 

Pinger        pinger;
BlynkTimer    timer;
WiFiUDP       ntpUDP;
NTPClient     timeClient(ntpUDP, "pool.ntp.org", 3600, 60000);
WiFiClient    espClient;
PubSubClient  client(espClient);


const char*   subscribed_topic      = "raspberryToEsp/relayControl";
const char*   subscribed_topic_1    = "raspberryToEsp/automation";
const char*   subscribed_topic_2    = "raspberryToEsp/motionHold";

const char*   publish_topic_hello   = "espToRaspberry/hello";
const char*   publish_topic_relayState = "espToRaspberry/motionState";
const char*   publish_topic_1       = "espToRaspberry/motionTimeout";
const char*   publish_topic_motion  = "espToRaspberry/motionESP01";
const char*   publish_topic_ssid    = "espToRaspberry/ssid";


String        formattedDate;
String        dayStamp;
String        timeStamp;
bool          automatic             = true;
bool          motion_detected       = false;
bool          reset_motion_timeout  = false;
long          motion_started_ts     = 0;
long          motion_elapse_sec     = 0;
long          motion_elapse         = 0;
long          motion_timeout_sec    = 0;
int           motion_hold           = 10;

unsigned long lastMsg               = 0;
unsigned long lastMsg_timeout       = 0;

#define       MSG_BUFFER_SIZE  (50)
char          msg_relayState[MSG_BUFFER_SIZE];
char          msg_timeout[MSG_BUFFER_SIZE];
char          msg_motion[MSG_BUFFER_SIZE];
int           value                 = 0;

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  // condition to handle automation toggle
  if(strcmp(topic,subscribed_topic_1)==0){
      if ((char)payload[0] == 'a')  automatic = true;
      else                          automatic = false;
  }

 if(strcmp(topic,subscribed_topic)==0){
  // Switch on the LED if an 1 was received as first character
  if ((char)payload[0] == '1') {
    Serial.println("Turn ON RELAY");
    if(!automatic) digitalWrite(RELAY, HIGH);   
  } else {
    Serial.println("Turn OFF RELAY");
    if(!automatic) digitalWrite(RELAY, LOW); 
  }
 }

   if(strcmp(topic,subscribed_topic_2)==0){
     String recv_payload = String(( char *) payload);
     motion_hold = recv_payload.toInt();
    Serial.print("Motion Hold as Int:");
     Serial.println(motion_hold);

  }

}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...\n");
    // Create a random client ID
    String clientId = "ESP01_MKE2-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println("MQTT broker connected");
      // Once connected, publish an announcement...
      client.publish(publish_topic_hello, "hello from ESP01 MKE2");
      // client.publish(publish_topic_ssid, ssid);
      // ... and resubscribe
      client.subscribe(subscribed_topic);
      client.subscribe(subscribed_topic_1);
      client.subscribe(subscribed_topic_2);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
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
      Serial.println();
    }
    else
    {
      Serial.printf("Request timed out.\n");
    }
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
    return true;
  });
  
}

void checkDeviceState(){
  bool relayState   = digitalRead(RELAY);
  bool ledState     = digitalRead(LED02);
  bool motionState  = digitalRead(MOTION);
  Serial.print("\nAuto: "); Serial.print(automatic);
  Serial.print("\tRelayState: "); Serial.print(relayState);
  Serial.print("\t\tMotion: "); Serial.println(motionState);

  if(motionState){
      snprintf (msg_motion, MSG_BUFFER_SIZE, "Motion Detected ESP01");
   }
   else{
      snprintf (msg_motion, MSG_BUFFER_SIZE, "Motion Not Detected ESP01");
    }

  if(relayState){
      snprintf (msg_relayState, MSG_BUFFER_SIZE, "Relay is #1");
   }else{
      snprintf (msg_relayState, MSG_BUFFER_SIZE, "Relay is #0");   
    }

  if(motionState){
    reset_motion_timeout  = true;
    if(!motion_detected){
      motion_started_ts     = millis();
      motion_detected       = true;
      if(automatic){
          digitalWrite(RELAY,HIGH);
        }
         
    }
    if(reset_motion_timeout) motion_started_ts = millis();
  }
  else{
    reset_motion_timeout  = false;
    
  }

  if(motion_detected){
    motion_elapse = millis() - motion_started_ts;
    motion_elapse_sec = motion_elapse / 1000;
    motion_timeout_sec = motion_hold - motion_elapse_sec;
    if(motion_timeout_sec <= 0) {
        motion_timeout_sec = 0;
    }
    // Serial.print("Motion Hold (s): "); Serial.println(motion_timeout_sec);
    String rt = getReadableTime(motion_timeout_sec);
    // Serial.print("ReadableTime: "); Serial.println(rt);

    char buff[MSG_BUFFER_SIZE];
    rt.toCharArray(buff, MSG_BUFFER_SIZE);
    snprintf (msg_timeout, MSG_BUFFER_SIZE, "Timed Out in T -- %s", buff);
    if(automatic) digitalWrite(RELAY,HIGH);

    if(automatic && motion_timeout_sec <= 0){
          digitalWrite(RELAY,LOW);
          client.publish(publish_topic_1, "MKE2 Light Off");
          motion_detected = false;
    }
  }
  else{
    // if no more motion but still in automatic
    if(automatic) digitalWrite(RELAY,LOW);
  }
  // client.publish(publish_topic_motion, msg_motion);
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

void get_uptime(){
  String uptime = String(uptime_formatter::getUptime()); 
}


void printTimeNTP(){
  timeClient.update();
  formattedDate = timeClient.getFormattedDate();
  // Extract date
  int splitT = formattedDate.indexOf("T");
  dayStamp = formattedDate.substring(0, splitT);
  String dateTime = "Now " + timeClient.getFormattedTime() + "\t\t" + dayStamp;

}

void push_motionstate(){
    client.publish(publish_topic_motion, msg_motion);
}

void push_relaystate(){
    client.publish(publish_topic_relayState, msg_relayState);
}

void push_motiontimeout(){
  client.publish(publish_topic_1, msg_timeout);
}

void push_ssid(){
  client.publish(publish_topic_ssid, ssid);
}

// the setup function runs once when you press reset or power the board
void setup() {
  // initialize digital pin LED_BUILTIN as an output.
  Serial.begin(115200);
  Serial.println("Welcome to ESP01s MKE2");
  pinMode(LED02, OUTPUT);
  pinMode(RELAY, OUTPUT);
  pinMode(MOTION, INPUT);
  digitalWrite(RELAY, LOW);   
  digitalWrite(LED02, LOW);
  delay(1000);

  long connectingTime = millis();
  
  WiFi.begin(ssid, pass); 
  int elapse = 0;
  //  first try with ssid_0
    while (WiFi.status() != WL_CONNECTED) {
      delay(1000);
      elapse = 20000 - (millis() - connectingTime);
      Serial.print("Connecting to ssid_0 in ");
      Serial.println(elapse/1000);
      if(millis()-connectingTime > 20000){
          Serial.println("Cant connect to ssid_0! Changing WiFi SSID to ssid_1");
          ssid = ssid_1;
          break;
      }
  }
  connectingTime = millis();
  //  second try with ssid_1
  WiFi.begin(ssid, pass);
    while (WiFi.status() != WL_CONNECTED) {
      delay(1000);
      elapse = 20000 - (millis() - connectingTime);
      Serial.print("Connecting to ssid_1 in ");
      Serial.println(elapse/1000);
      if(millis()-connectingTime > 20000){
          Serial.println("Cant connect to ssid_1! Restarting now...");
          delay(2000);
          ESP.restart();
      }
  }


  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);

  timeClient.begin();
  timeClient.setTimeOffset(28800);

  // timer.setInterval(10000L, get_ping);
  timer.setInterval(1000L, checkDeviceState);
  timer.setInterval(5000L, push_motionstate);
  timer.setInterval(1000L, push_motiontimeout);
  timer.setInterval(5000L, push_relaystate);
  timer.setInterval(10000L, push_ssid);
  // timer.setInterval(60000L, get_uptime);
  // timer.setInterval(1000L, printTimeNTP);
}

// the loop function runs over and over again forever
void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  timer.run();
}
