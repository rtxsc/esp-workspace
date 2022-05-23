#include <WiFiClientSecure.h>
#include <ArduinoJson.h>

const char* ssid     = "NPRDC CELCOM M2";
const char* password = "nprdc1234";
const char*  server = "https://maps.googleapis.com";  // Server URL
const int httpPort = 443;
String urlPiece = "/maps/api/geocode/json?latlng=";
String urlKey = "&key=AIzaSyD5Veclfg27JklUnd_6jD5OtktCfmQg8Gc";

WiFiClientSecure client; // 23.05.2022 Monday

void setup() {
  client.setInsecure(); // skip verification for HTTPS certificate
  Serial.begin(115200);
  Serial.print("Attempting to connect to SSID: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address : ");
  Serial.println(WiFi.localIP());


  //  1.449183,110.448915
  float lat0 = 1.449183;
  float lon0 = 110.448915;

  //  1.4870° N, 110.3416° E
  float lat1 = 1.4870;
  float lon1 = 110.3416;

  //1.5271797298812677, 110.36796510853041
  float lat2 = 1.5271797298812677;
  float lon2 = 110.36796510853041;
  
  Serial.print("\n1st get_formatted_address call\n");
  get_formatted_address(lat0,lon0);
  delay(2000);
  Serial.print("\n2nd get_formatted_address call\n");
  get_formatted_address(lat1,lon1);
  delay(2000);
  Serial.print("\n3rd get_formatted_address call\n");
  get_formatted_address(lat2,lon2);
  delay(2000);
}

void get_formatted_address(float lat, float lon){
  String url = urlPiece + lat + ',' + lon + urlKey;

  while (!client.connect(server, httpPort)) {
    Serial.printf("[ERROR]: Connection to %s failed!", server);
    Serial.println("Waiting 2 secs");
    delay(2000);
    Serial.println("Retrying connecting...");
  }
  
  Serial.printf("[SUCCESS]: Connected to Google Maps server: %s \n",server);
  delay(2000);
  
  Serial.print("\nSending HTTPS Request.\n");
  Serial.println(server + url + "\n");
  client.print(String("GET ") + url + "\r\n" +
               "Host: " + server + "\r\n" +
               "Connection: close\r\n\r\n");

////  The 4 lines belows were coming from WiFiClientSecure example
////  Make a HTTP request:
//  client.println("GET https://maps.googleapis.com/maps/api/geocode/json?latlng=1.449183,110.448915&key=AIzaSyD5Veclfg27JklUnd_6jD5OtktCfmQg8Gc");
//  client.println("Host: maps.googleapis.com");
//  client.println("Connection: close");
//  client.println();

    while (client.connected()) {
      String line = client.readStringUntil('\n');
      if (line == "\r") {
        Serial.println("[INFO] Headers received successfully"); //  skip response header / end of header found
        break;
      }
    }
// if there are incoming bytes available
// from the server, read them and print them:
//    while (client.available()) {
//      char c = client.read();
//      Serial.write(c);
//    }
   
    Serial.println("Performing ArduinoJSON stuff now...");
    //Use arduinojson.org/assistant to compute the capacity.
    const size_t bufferSize = JSON_ARRAY_SIZE(1) + 2 * JSON_OBJECT_SIZE(2) + JSON_OBJECT_SIZE(3) + 120;
    
    //Allocate JsonBuffer
    DynamicJsonBuffer jsonBuffer(bufferSize);
    
    //Parse JSON object
    JsonObject& root = jsonBuffer.parseObject(client);
    client.stop();
    Serial.println("client.stop() called");
    delay(1000);
    if (!root.success()) {
      Serial.println(F("Parsing failed!"));
      return;
    }else{
      Serial.println(F("Parsing successful!"));
    }

    JsonObject& results0 = root["results"][0];
    String results0_formatted_address = results0["formatted_address"];
    for(int i = 0 ; i < results0_formatted_address.length() ; i++)
      Serial.print("#");
    Serial.println();
    Serial.println(results0_formatted_address);
    for(int i = 0 ; i < results0_formatted_address.length() ; i++)
      Serial.print("#");
    Serial.println();
  
}

void loop() {

}
