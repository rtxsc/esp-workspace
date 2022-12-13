/*
 *  This sketch sends a message to a TCP server
 *
 */

#include <WiFi.h>
#define ONBOARD_LED 2

const char* ssid = "UiTM WiFi IoT"; // "Maxis Postpaid 128 5G";

void setup(){
    Serial.begin(115200);
    pinMode(ONBOARD_LED, OUTPUT);
    delay(1000);

    WiFi.mode(WIFI_STA); //Optional
    WiFi.begin(ssid);
    Serial.println("\nConnecting");

    while(WiFi.status() != WL_CONNECTED){
        Serial.println(".");
        delay(500);
    }

    Serial.println("\nConnected to the WiFi network");
    Serial.print("Local ESP32 IP: ");
    Serial.println(WiFi.localIP());
}

void loop()
{
    const uint16_t port = 443;
    const char * host = "blynk.cloud"; // ip or dns

    Serial.print("Connecting to ");
    Serial.println(host);

    // Use WiFiClient class to create TCP connections
    WiFiClient client;

    if (!client.connect(host, port)) {
        Serial.println("Connection failed.");
        Serial.println("Waiting 5 seconds before retrying...");
        delay(5000);
        return;
    }
    Serial.print("Status: ");
    Serial.print(host);
    Serial.print(" connected at port ");
    Serial.println(port);
    // This will send a request to the server
    //uncomment this line to send an arbitrary string to the server
    //client.print("Send this data to the server");
    //uncomment this line to send a basic document request to the server
//    client.print("GET / HTTP/1.1\n\n");
    client.print("GET /echo HTTP/1.1\n\n");
    digitalWrite(ONBOARD_LED, HIGH);

  int maxloops = 0;

  //wait for the server's reply to become available
  while (!client.available() && maxloops < 1000)
  {
    maxloops++;
    delay(1); //delay 1 msec
  }
  if (client.available() > 0)
  {
    //read back one line from the server
    String line = client.readStringUntil('\r');
    Serial.println(line);
  }
  else
  {
    Serial.println("client.available() timed out ");
  }

    Serial.println("Closing connection.");
    client.stop();
    digitalWrite(ONBOARD_LED, LOW);
    Serial.println("Waiting 2 seconds before restarting...");
    delay(2000);
}
