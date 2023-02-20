#include <Wire.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <Arduino_JSON.h>
#include "uptime_formatter.h"
#include "uptime.h"
#include <WiFiClientSecure.h>

// #define ESP32S2 // comment this line if uploading to ESP32C3
#define LCD_DISABLED // uncomment this for ESP32C3

#ifdef ESP32S2
  #include <Adafruit_NeoPixel.h>
  #include <Wire.h>
  #include "rgb_lcd.h"
  #define RGB         18 
  #define NUMPIXELS   1 
  #define DELAYVAL    500 
  Adafruit_NeoPixel   pixels(NUMPIXELS, RGB);
  rgb_lcd             lcd;
#endif

#define DEBUG_SERIAL
#define ONBOARD_LED 2
#define RED 3
#define GRN 4
#define BLU 5
#define I2C_SDA                 41
#define I2C_SCL                 40

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

const char* ca_cert = \
"-----BEGIN CERTIFICATE-----\n" \
"MIIFNDCCBNugAwIBAgIQBNn/61aAiBJfiezi1D3EqzAKBggqhkjOPQQDAjBKMQsw\n" \
"CQYDVQQGEwJVUzEZMBcGA1UEChMQQ2xvdWRmbGFyZSwgSW5jLjEgMB4GA1UEAxMX\n" \
"Q2xvdWRmbGFyZSBJbmMgRUNDIENBLTMwHhcNMjIwNTExMDAwMDAwWhcNMjMwNTEw\n" \
"MjM1OTU5WjB1MQswCQYDVQQGEwJVUzETMBEGA1UECBMKQ2FsaWZvcm5pYTEWMBQG\n" \
"A1UEBxMNU2FuIEZyYW5jaXNjbzEZMBcGA1UEChMQQ2xvdWRmbGFyZSwgSW5jLjEe\n" \
"MBwGA1UEAxMVc25pLmNsb3VkZmxhcmVzc2wuY29tMFkwEwYHKoZIzj0CAQYIKoZI\n" \
"zj0DAQcDQgAE8IMERGcUe5+tcpaPvSatP0spInqjqn9ad7L87jMk+KSI0rYz+rrM\n" \
"wUdeLYN1Bsvw1rKHcMrKhZfKZKN96DhzXaOCA3YwggNyMB8GA1UdIwQYMBaAFKXO\n" \
"N+rrsHUOlGeItEX62SQQh5YfMB0GA1UdDgQWBBRQYqmZxXhHEHWGsv4FZN38jL1A\n" \
"2jA+BgNVHREENzA1ghVzbmkuY2xvdWRmbGFyZXNzbC5jb22CDiouZ2VvYXBpZnku\n" \
"Y29tggxnZW9hcGlmeS5jb20wDgYDVR0PAQH/BAQDAgeAMB0GA1UdJQQWMBQGCCsG\n" \
"AQUFBwMBBggrBgEFBQcDAjB7BgNVHR8EdDByMDegNaAzhjFodHRwOi8vY3JsMy5k\n" \
"aWdpY2VydC5jb20vQ2xvdWRmbGFyZUluY0VDQ0NBLTMuY3JsMDegNaAzhjFodHRw\n" \
"Oi8vY3JsNC5kaWdpY2VydC5jb20vQ2xvdWRmbGFyZUluY0VDQ0NBLTMuY3JsMD4G\n" \
"A1UdIAQ3MDUwMwYGZ4EMAQICMCkwJwYIKwYBBQUHAgEWG2h0dHA6Ly93d3cuZGln\n" \
"aWNlcnQuY29tL0NQUzB2BggrBgEFBQcBAQRqMGgwJAYIKwYBBQUHMAGGGGh0dHA6\n" \
"Ly9vY3NwLmRpZ2ljZXJ0LmNvbTBABggrBgEFBQcwAoY0aHR0cDovL2NhY2VydHMu\n" \
"ZGlnaWNlcnQuY29tL0Nsb3VkZmxhcmVJbmNFQ0NDQS0zLmNydDAMBgNVHRMBAf8E\n" \
"AjAAMIIBfAYKKwYBBAHWeQIEAgSCAWwEggFoAWYAdgDoPtDaPvUGNTLnVyi8iWvJ\n" \
"A9PL0RFr7Otp4Xd9bQa9bgAAAYCxtrYaAAAEAwBHMEUCIQD1/3wfzNfE4EaDFKYc\n" \
"j21kBNt+5y6s9SZJLInD9h7mvAIgEOgai3WtDk+kzgAA0+5D1GmDw9fKVMRmOTk+\n" \
"niKnuK8AdQA1zxkbv7FsV78PrUxtQsu7ticgJlHqP+Eq76gDwzvWTAAAAYCxtrZR\n" \
"AAAEAwBGMEQCIBhaXWb+CTgAqbGsMVXWbWaxy4rsXbnY+PXpdCaup0yHAiA7TuJl\n" \
"pSXCa0SbG6iBAd/KzW/gdrgDUt1Gw8kvXyNGWwB1ALc++yTfnE26dfI5xbpY9Gxd\n" \
"/ELPep81xJ4dCYEl7bSZAAABgLG2tjwAAAQDAEYwRAIgVYZLggdL3rlxJ94yK5bd\n" \
"suDu3GBXv2UNf+9ndfq8QX4CIEemUgGZmFt42f7CjU7DNSidT8RPowzRYY2PK/jA\n" \
"mCc0MAoGCCqGSM49BAMCA0cAMEQCID3g3Zahwe8HIbyEGCRjz/fR8vIp7+svso3N\n" \
"QltwkIqeAiA4kg9eacmaF5hCfPmIbhFT+sUfty8Yv+vxqUsI29Ga8w==\n" \
"-----END CERTIFICATE-----\n";


void on_onboard_led();
void off_onboard_led();

// const char* ssid = "UiTM WiFi IoT"; // "Maxis Postpaid 128 5G";
// const char* pass = "";

char ssid[] = "Maxis_128_5G";
char pass[] = "respironics"; // leave this empty as this is an open network

// const char* ssid = "Robotronix"; // "Maxis Postpaid 128 5G";
// const char* pass = "robotroxian";

bool openNetwork = false;

String openWeatherMapApiKey = "dbd7235bcc77e5896c73e975d013debe";
String city = "Kuching";
String countryCode = "MY";
unsigned long lastTime = 0;
unsigned long interval = 5000;
// https://api.geoapify.com/v1/geocode/reverse?lat=51.21709661403662&lon=6.7782883744862374&apiKey=aad49482771c41c8bd927acac874e28a
// curl --location--request GET 'https://api.geoapify.com/v1/geocode/reverse?lat=51.21709661403662&lon=6.7782883744862374&apiKey=aad49482771c41c8bd927acac874e28a'

String jsonBufferGeo= "None";

String jsonAddress  = "None";
String jsonAddress1  = "None";

String jsonBuffer   = "None";
String jsonString   = "None";
String jsonHumid    = "None";
String jsonWeather  = "None";

float tempK ; 
float tempC ;
float humid ; 
String mac_str = "None";
String esp_model = "None";


// const char* server = "https://api.geoapify.com/v1/geocode/reverse?lat=1.4471437206974116&lon=110.45102315640258&apiKey=aad49482771c41c8bd927acac874e28a";

WiFiClientSecure client_https;

const char*  server = "api.geoapify.com";  // Server URL
// const char*  server = "maps.googleapis.com";  // Server URL

const int httpPort = 443;
String urlPiece = "/v1/geocode/reverse?lat=1.4471437206974116&lon=110.45102315640258";
String urlKey = "&apiKey=aad49482771c41c8bd927acac874e28a";

void setup(){
    #ifdef DEBUG_SERIAL
    Serial.begin(115200);
    #endif
    mac_str = WiFi.macAddress();
    const char* mac_addr = mac_str.c_str();
    Serial.print("MAC (String):");
    Serial.println(mac_str);
    Serial.printf("MAC (const char): %s\n", mac_addr);

    if(strcmp(mac_addr,"7C:DF:A1:AF:AA:B4")==0) esp_model = "ESP32C3-1";
    if(strcmp(mac_addr,"7C:DF:A1:AF:AC:FC")==0) esp_model = "ESP32C3-2";
    if(strcmp(mac_addr,"7C:DF:A1:AF:AB:00")==0) esp_model = "ESP32C3-3";
    if(strcmp(mac_addr,"7C:DF:A1:AF:AD:24")==0) esp_model = "ESP32C3-4"; // aziz
    if(strcmp(mac_addr,"7C:DF:A1:AF:AB:C0")==0) esp_model = "ESP32C3-5"; // xbox
    if(strcmp(mac_addr,"7C:DF:A1:AF:AC:B8")==0) esp_model = "ESP32C3-6"; // aniq

    if(strcmp(mac_addr,"7C:DF:A1:00:72:C6")==0) esp_model = "ESP32S2-1";
    if(strcmp(mac_addr,"7C:DF:A1:00:BB:7C")==0) esp_model = "ESP32S2-2";
    if(strcmp(mac_addr,"7C:DF:A1:00:BA:9E")==0) esp_model = "ESP32S2-3";
    if(strcmp(mac_addr,"7C:DF:A1:00:6D:AA")==0) esp_model = "ESP32S2-4";
    if(strcmp(mac_addr,"7C:DF:A1:00:8D:74")==0) esp_model = "ESP32S2-5";
    if(strcmp(mac_addr,"7C:DF:A1:00:BA:BE")==0) esp_model = "ESP32S2-6";
    if(strcmp(mac_addr,"7C:DF:A1:00:A7:0C")==0) esp_model = "ESP32S2-7";
    if(strcmp(mac_addr,"7C:DF:A1:00:A7:38")==0) esp_model = "ESP32S2-8";

    if(strcmp(mac_addr,"AC:67:B2:25:85:78")==0) esp_model = "ESP32DEV-0";
    if(strcmp(mac_addr,"C8:2B:96:B9:A9:58")==0) esp_model = "ESP32DEV-1";
    if(strcmp(mac_addr,"9C:9C:1F:E3:85:3C")==0) esp_model = "ESP32DEV-2";
    if(strcmp(mac_addr,"84:CC:A8:5E:6E:E8")==0) esp_model = "ESP32DEV-3";
    if(strcmp(mac_addr,"9C:9C:1F:C5:94:24")==0) esp_model = "ESP32DEV-4";
    if(strcmp(mac_addr,"84:0D:8E:E2:D6:D8")==0) esp_model = "ESP32DEV-5";


    Serial.printf("[setup] %s Found!\n",esp_model);
    Serial.printf("[setup] MAC: %s\n", mac_addr);
    mac_str.remove(2,1); // remove the first : from MAC 

    pinMode(ONBOARD_LED, OUTPUT);
    pinMode(RED, OUTPUT);
    pinMode(GRN, OUTPUT);
    pinMode(BLU, OUTPUT);

    #ifndef LCD_DISABLED
    Wire.begin();
    // Wire.begin(I2C_SDA, I2C_SCL);

    lcd.begin(16, 2);
    lcd.createChar(1, wave_right); // create block character
    lcd.createChar(2, wave_left); // create block character
    lcd.createChar(3, right_arrow); // create block character
    lcd.createChar(4, degree_symbol); // create block character
    lcd.setCursor(0,0);
    lcd.print(esp_model);
    lcd.setCursor(0,1);
    lcd.print("Connecting WiFi");
    delay(1000);
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("MAC Address");
    lcd.setCursor(0,1);
    lcd.print(mac_str);
    delay(2000);
    #endif
    // WiFi.mode(WIFI_STA); //Optional
    #ifdef ESP32S2
      try_wifi_connect(); 
    #else // the following will be used by ESP32C3 test unit
      WiFi.begin(ssid,pass);
      #ifdef DEBUG_SERIAL
      Serial.print("\nConnecting to ");
      Serial.println(ssid);
      #endif

      int i = 0;
      while(WiFi.status() != WL_CONNECTED){
          #ifdef DEBUG_SERIAL
          Serial.print("|");
          #endif

          if(i % 2 == 0){
            #ifdef ESP32S2
            redOn();
            #endif
          }else{
            #ifdef ESP32S2
            rgbOff();
            #endif
          }

          if(i % 30 == 0){
            #ifdef DEBUG_SERIAL
            Serial.println();
            #endif
          }
          if(i == 10){
            on_blu();
            delay(1000);
            ESP.restart();
          }
          on_red();
          on_onboard_led();
          delay(100);
          off_rgb();
          off_onboard_led();
          delay(100);
          i++;
      }
    #endif

    #ifdef DEBUG_SERIAL
    Serial.println("\nConnected to the WiFi network");
    Serial.print("Local ESP32 IP: ");
    Serial.println(WiFi.localIP());   
    #endif

    #ifdef ESP32S2
      pixels.begin(); 
      loopRGB();
    #endif

    client_https.setInsecure(); // dedicated for Google Maps API or GeoApify | 20 Feb 2023 Success
    // client_https.setCACert(ca_cert);       

    int  conn;
    int chip_id = ESP.getEfuseMac();;
    Serial.printf("  Flash Chip id = %08X\t", chip_id);
    Serial.println();
    String body = "ChipId=" + String(chip_id) + "&SentBy=" + "your_name";
    int body_len = body.length();
    Serial.println(".....");  
    request_http();
    request_https();

}

/*
http://api.openweathermap.org/data/2.5/weather?q=Kuching,MY&APPID=dbd7235bcc77e5896c73e975d013debe

dbd7235bcc77e5896c73e975d013debe
*/

void loop() {
  // do nothing
}

void request_http(){
  Serial.print("\nSending HTTP Request.\n");

  String serverPath = "http://api.openweathermap.org/data/2.5/weather?q=" + city + "," + countryCode + "&APPID=" + openWeatherMapApiKey;
  jsonBuffer = httpGETRequest(serverPath.c_str());
  Serial.printf("[OpenWeather] HTTP payload out is \n");
  Serial.println(jsonBuffer);
  Serial.printf("[End of payload]\n");
  JSONVar myObject = JSON.parse(jsonBuffer);
  if (JSON.typeof(myObject) == "undefined") {
    #ifdef DEBUG_SERIAL
    Serial.println("[Open Weather Map] Parsing input failed!");
    #endif
    return;
  }
  Serial.println("[Open Weather Map] Parsing input successful!");
  Serial.print("myObject object before calling stringify:");  Serial.println(myObject);
  jsonWeather = JSON.stringify(myObject["weather"][0]["description"]);
  Serial.print("myObject object after calling stringify:");  Serial.println(myObject);
  Serial.print("Weather:");  Serial.println(jsonWeather);

}

void request_https(){
  String url = urlPiece + urlKey;
  String serverGeoapify = "https://" + url;

  while (!client_https.connect(server, httpPort)) {
    Serial.printf("[ERROR]: Connection to %s failed!\n", server);
    Serial.println("Waiting 2000 ms");
    Serial.print("Retrying connecting");
    for(int i=0; i<20; i++){
      Serial.print(".");
      delay(100);
    }
  }
  
  Serial.printf("[SUCCESS]: Connected to GeoAPI  server: %s \n",server);
  
  Serial.print("\nSending HTTPS Request.\n");
  Serial.println(String("https://") + server + url + "\n");

  // client_https.println("GET "+url+" HTTP/1.1");
  // client_https.println("Host: api.geoapify.com");
  // client_https.println("Connection: close");
  // client_https.println();

  // while (client_https.connected()) {
  //   String line = client_https.readStringUntil('\n');
  //   if (line == "\r") {
  //     Serial.println("[SUCCESS] Headers received successfully"); //  skip response header / end of header found
  //     Serial.println("Printing output");
  //     Serial.println(line);
  //     break;
  //   }else{
  //     Serial.print("-"); // [WARNING] Headers not received!
  //   }
  // }

  // String out = "{}";
  // while (client_https.available()) {
  //   // char c = client_https.read();
  //   // Serial.write(c);
  //   out = client_https.readString();
  // }
  // int length_str = out.length();
  // out.remove(0,3);
  // out.remove((length_str-1),1);

  String jsonBufferGeo = httpsGETRequest(serverGeoapify.c_str());

  Serial.printf("HTTPS payload out is \n");
  Serial.println(jsonBufferGeo);
  Serial.printf("[End of payload]\n");

  JSONVar rev_geo = JSON.parse(jsonBufferGeo);

  if (JSON.typeof(rev_geo) == "undefined") {
    #ifdef DEBUG_SERIAL
    Serial.println("[Reverse Geocode] Parsing input failed!");
    delay(1000);
    #endif
    return;
  }
  Serial.println("[Reverse Geocode] Parsing input successful!");
  Serial.print("rev_geo object before calling stringify:"); Serial.println(rev_geo);

  jsonAddress1 = JSON.stringify(rev_geo['features'][0]['properties']['address_line1']);
  Serial.print("Address 1:");  Serial.println(jsonAddress1);

  Serial.print("rev_geo object after calling stringify:");  Serial.println(rev_geo);

}

/*
import requests
import json
from requests.structures import CaseInsensitiveDict

lat1 = 1.4471437206974116
lon1 = 110.45102315640258

lat2 = 1.4560609713111177
lon2 = 110.42437033120086


def get_address(lat,lon):
    url = "https://api.geoapify.com/v1/geocode/reverse?lat="+str(lat)+"&lon="+str(lon)+"&apiKey=aad49482771c41c8bd927acac874e28a"

    headers = CaseInsensitiveDict()
    headers["Accept"] = "application/json"
    resp = requests.get(url, headers=headers)
    # print(resp.status_code)
    data = json.loads(resp.text)
    # print(resp.json().items())
    print("Name:"       , data['features'][0]['properties']['name'])
    print("Country:"    , data['features'][0]['properties']['country'])
    print("Formatted:"  , data['features'][0]['properties']['formatted'])
    print("Address 1:"  , data['features'][0]['properties']['address_line1'])
    print("Address 2:"  , data['features'][0]['properties']['address_line2'])

get_address(lat1,lon1)
get_address(lat2,lon2)
*/

// void loop()
// {
//     Serial.printf("%s Found!\n",esp_model);
//     Serial.println(mac_str);
//     #ifndef LCD_DISABLED
//     lcd.clear();
//     lcd.setCursor(0,0);
//     lcd.print(esp_model);
//     lcd.setCursor(0,1);
//     lcd.print(mac_str);
//     delay(1000);
//     #endif

//   int RSSI_dBm =  WiFi.RSSI();
//   String rssi_state = get_rssi_state(RSSI_dBm);
//   #ifndef LCD_DISABLED
//   display_uptime_top_row();
//   #endif    
//     // Send an HTTP GET request
//   if ((millis() - lastTime) > interval) {
//     // Check WiFi connection status
//     if(WiFi.status()== WL_CONNECTED){ 

//       on_grn();    
//       on_onboard_led();
//       #ifdef ESP32S2
//       greenOn();
//       #endif
//       String serverPath = "http://api.openweathermap.org/data/2.5/weather?q=" + city + "," + countryCode + "&APPID=" + openWeatherMapApiKey;
//       String serverGeoapify = "https://api.geoapify.com/v1/geocode/reverse?lat=51.21709661403662&lon=6.7782883744862374&apiKey=aad49482771c41c8bd927acac874e28a";
//       // https://api.geoapify.com/v1/geocode/reverse?lat=1.4471437206974116&lon=110.45102315640258&apiKey=aad49482771c41c8bd927acac874e28a
//     client_https.connect(serverGeoapify.c_str(),443);
//     client_https.println("GET " + serverGeoapify +"/ HTTP/1.0");


//       // jsonBufferGeo = httpGETRequest(serverGeoapify.c_str());
      
//       jsonBuffer = httpGETRequest(serverPath.c_str());
//       #ifdef DEBUG_SERIAL
//       // Serial.println(jsonBuffer); // print all JSON output
//       Serial.println(jsonBufferGeo); // print all JSON output

//       #endif
//       JSONVar myObject = JSON.parse(jsonBuffer);
//       // JSONVar rev_geo = JSON.parse(jsonBufferGeo);

//       // if (JSON.typeof(rev_geo) == "undefined") {
//       //   #ifdef DEBUG_SERIAL
//       //   Serial.println("[Reverse Geocode] Parsing input failed!");
//       //   delay(1000);
//       //   #endif
//       //   return;
//       // }

  
//       // JSON.typeof(jsonVar) can be used to get the type of the var
//       if (JSON.typeof(myObject) == "undefined") {
//         #ifdef DEBUG_SERIAL
//         Serial.println("[Open Weather Map] Parsing input failed!");
//         #endif
//         return;
//       }

//       // 0K − 273.15

//       jsonString = JSON.stringify(myObject["main"]["temp"]);
//       jsonHumid = JSON.stringify(myObject["main"]["humidity"]);

//       tempK = jsonString.toFloat(); 
//       tempC = tempK - 273.15;
//       humid = jsonHumid.toFloat(); 

//       jsonWeather = JSON.stringify(myObject["weather"][0]["description"]);
//       // String jsonWeather = "thunderstorm with light rain";
  
//       #ifdef DEBUG_SERIAL
//       Serial.print("JSON object = ");
//       Serial.println(myObject);

//       // Serial.print("Reverse Geo object = ");
//       // Serial.println(rev_geo);
//       // Serial.print("Temperature: ");
//       // Serial.println(tempC);
//       // Serial.print("Pressure: ");
//       // Serial.println(myObject["main"]["pressure"]);
//       // Serial.print("Humidity: ");
//       // Serial.println(myObject["main"]["humidity"]);
//       // Serial.print("Wind Speed: ");
//       // Serial.println(myObject["wind"]["speed"]);
//       // Serial.print("Weather: ");
//       // Serial.println(myObject["weather"][0]["description"]);
//       // Serial.print("Feels like: ");
//       // Serial.println(myObject["main"]["feels_like"]);
//       #endif
//     }
//     else {
//       #ifdef DEBUG_SERIAL
//       Serial.println("WiFi Disconnected");
//       #endif

//       #ifndef LCD_DISABLED
//       lcd.clear();
//       lcd.setCursor(0,0); // row 0, column 0
//       lcd.print("WiFi Disconnect"); // print connected SSID
//       lcd.setCursor(0,1); // row 0, column 0
//       lcd.print("Restart ESP Now"); // print connected SSID
//       #endif
//       on_blu();
//       #ifdef ESP32S2
//       blueOn();
//       #endif
//       delay(1000);
//       ESP.restart();
//     }
//     lastTime = millis();
//   }
//   // taken out from if(WiFi.status()== WL_CONNECTED) logic 20 Jan 2023
//   // solving the weird frozen issue when i2c LCD attached 

//   #ifndef LCD_DISABLED
//   if(millis() % 2 == 0){
//         lcd.setCursor(0,1);
//         lcd.print(jsonWeather);
//         // int str_length = jsonWeather.length();
//         // if(str_length > 16){
//         //   for (int positionCounter = 0; positionCounter < 13; positionCounter++) {
//         //     // scroll one position left:
//         //     lcd.scrollDisplayLeft();
//         //     // wait a bit:
//         //     delay(150);
//         //   }
//         // }
//       }
//       else if(millis() % 5 == 0){
//         lcd.clear();
//         lcd.setCursor(0,0);
//         lcd.print(esp_model);
//         lcd.setCursor(0,1); // row 0, column 0
//         lcd.print(mac_str); // print connected SSID
//       }
//       else if(millis() % 11 == 0){

//       }
//       else{
//         lcd.clear();
//         lcd.print(rssi_state);
//         lcd.setCursor(0,1);
//         lcd.print(String(tempC) + " " + String(humid)+"%");        
//       }
//   #endif
//   off_rgb();
//   #ifdef ESP32S2
//   rgbOff();
//   #endif
//   off_onboard_led();
//   delay(1000);

// } // end of void loop

String httpsGETRequest(const char* serverName) {
  HTTPClient https;
    
  // Your Domain name with URL path or IP address with path
  https.begin(client_https, serverName);
  
  // Send HTTP POST request
  int httpResponseCode = https.GET();
  
  String payload = "{}"; 
  
  if (httpResponseCode>0) {
    #ifdef DEBUG_SERIAL
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    #endif
    payload = https.getString();
  }
  else {
    #ifdef DEBUG_SERIAL
    Serial.print("HTTPS GET Error code: ");
    Serial.println(httpResponseCode);
    #endif

  }
  // Free resources
  https.end();

  return payload;
}

String httpGETRequest(const char* serverName) {
  WiFiClient client;
  HTTPClient http;
    
  // Your Domain name with URL path or IP address with path
  http.begin(client, serverName);
  
  // Send HTTP POST request
  int httpResponseCode = http.GET();
  
  String payload = "{}"; 
  
  if (httpResponseCode>0) {
    #ifdef DEBUG_SERIAL
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    #endif
    payload = http.getString();
  }
  else {
    #ifdef DEBUG_SERIAL
    Serial.print("HTTP GET Error code: ");
    Serial.println(httpResponseCode);
    #endif

  }
  // Free resources
  http.end();

  return payload;
}


void on_onboard_led(){
    digitalWrite(ONBOARD_LED, HIGH);
}

void off_onboard_led(){
    digitalWrite(ONBOARD_LED, LOW);
}

void on_red(){
  digitalWrite(RED, HIGH);
  digitalWrite(GRN, LOW);
  digitalWrite(BLU, LOW);
}

void on_grn(){
  digitalWrite(RED, LOW);
  digitalWrite(GRN, HIGH);
  digitalWrite(BLU, LOW);
}

void on_blu(){
  digitalWrite(RED, LOW);
  digitalWrite(GRN, LOW);
  digitalWrite(BLU, HIGH);
}

void off_rgb(){
  digitalWrite(RED, LOW);
  digitalWrite(GRN, LOW);
  digitalWrite(BLU, LOW);
}

#ifdef ESP32S2
void redOn(){
  pixels.setPixelColor(0, pixels.Color(127, 0, 0));
  pixels.show(); 
}
void greenOn(){
  pixels.setPixelColor(0, pixels.Color(0, 127, 0));
  pixels.show(); 
}
void blueOn(){
  pixels.setPixelColor(0, pixels.Color(0, 0, 127));
  pixels.show(); 
}

void rgbOff(){
  pixels.setPixelColor(0, pixels.Color(0, 0, 0));
  pixels.show(); 
  }


void loopRGB(){
 
  pixels.setPixelColor(0, pixels.Color(127, 0, 0));
  pixels.show();   
  delay(DELAYVAL); 
  pixels.setPixelColor(0, pixels.Color(0, 127, 0));
  pixels.show();   
  delay(DELAYVAL); 
  pixels.setPixelColor(0, pixels.Color(0, 0, 127));
  pixels.show();   
  delay(DELAYVAL); 
  pixels.setPixelColor(0, pixels.Color(0, 0, 0));
  pixels.show();   
  delay(DELAYVAL); 
   pixels.setPixelColor(0, pixels.Color(0, 0, 0));
  pixels.show();   
  delay(DELAYVAL); 
}
#endif

#ifdef ESP32S2
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
  #endif


String get_rssi_state(int rssi){
  String out;
  if(rssi > -30) out = "Perfect signal";
  else if(rssi > -50) out = "Excellent signal";
  else if(rssi > -60) out = "Good reliable signal";
  else if(rssi > -67) out = "Voice and Non-HD vid";
  else if(rssi > -70) out = "Light browsing and email";
  else if(rssi > -80) out = "Unstable connection";
  else                out = "Unlikely connection";
  return out;
}

#ifdef ESP32S2
void try_wifi_connect(){
    int init_index_right = 9; // init 8 + 1 offset
    int init_index_left = 6; // init 7 - 1 offset
    int column_index_right = init_index_right; 
    int column_index_left = init_index_left;

    int wl_count = 0;
    int connect_elapse = 10; // second unit
    Serial.printf("\n\nConnecting to %s\n\n", ssid);
    if(strcmp(pass,"")==0){
      Serial.println("This is an open network!");
      openNetwork = true;
    }else{
      Serial.println("This is a close network!");
      openNetwork = false;
    }
    WiFi.begin(ssid,pass);
    while (WiFi.status() != WL_CONNECTED) {
          Serial.printf("/");
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
            else if(millis()/1000 % 3 == 0 || millis()/1000 % 5 == 0){
                lcd.clear();
                lcd.setCursor(0, 0); // row 1, column 0
                lcd.print("Network Type");  
                lcd.setCursor(0, 1); // row 1, column 0
                if(openNetwork) lcd.print("    Open Network");  
                else            lcd.print("   Close Network");  
                delay(1000);
            }
            else{
                lcd.setCursor(0, 0); // row 1, column 0
                lcd.print(String(ssid));
            }
          }

      wl_count++;
      // vTaskDelay(100 / portTICK_PERIOD_MS);
      on_onboard_led();
      delay(50);
      off_onboard_led();  
      delay(50);
      
      if(wl_count > connect_elapse*10 && (WiFi.status() != WL_CONNECTED)){
          wl_count = 0;
          lcd.clear();
          lcd.setCursor(0, 0); // row 1, column 0
          lcd.print("Not Connected!");
          lcd.setCursor(0, 1); // row 1, column 0
          lcd.print("Restarting ESP32");
          // vTaskDelay(1000 / portTICK_PERIOD_MS);
          ESP.restart();
      }
    }
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("-WiFi Connected-");
      lcd.setCursor(0,1);
      lcd.print("--"+String(ssid)+"-");
      // vTaskDelay(1000 / portTICK_PERIOD_MS);
    
}
#endif