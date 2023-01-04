#include <Wire.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <Arduino_JSON.h>

#define ESP32S2

#ifdef ESP32S2
  #include <Adafruit_NeoPixel.h>
  #define RGB         18 
  #define NUMPIXELS   1 
  #define DELAYVAL    500 
  Adafruit_NeoPixel   pixels(NUMPIXELS, RGB);
#endif

#define DEBUG_SERIAL
#define ONBOARD_LED 2
#define RED 3
#define GRN 4
#define BLU 5
#define I2C_SDA                 41
#define I2C_SCL                 40

void on_onboard_led();
void off_onboard_led();

const char* ssid = "UiTM WiFi IoT"; // "Maxis Postpaid 128 5G";
String openWeatherMapApiKey = "dbd7235bcc77e5896c73e975d013debe";
String city = "Kuching";
String countryCode = "MY";
unsigned long lastTime = 0;
unsigned long interval = 2000;
String jsonBuffer;

void setup(){
    #ifdef DEBUG_SERIAL
    Serial.begin(115200);
    #endif
    Wire.begin(I2C_SDA, I2C_SCL);
    pinMode(ONBOARD_LED, OUTPUT);
    pinMode(RED, OUTPUT);
    pinMode(GRN, OUTPUT);
    pinMode(BLU, OUTPUT);
  
    // WiFi.mode(WIFI_STA); //Optional

    WiFi.begin(ssid);
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
    #ifdef DEBUG_SERIAL
    Serial.println("\nConnected to the WiFi network");
    Serial.print("Local ESP32 IP: ");
    Serial.println(WiFi.localIP());   
    #endif

    #ifdef ESP32S2
      pixels.begin(); 
      loopRGB();
    #endif

}

/*
http://api.openweathermap.org/data/2.5/weather?q=Kuching,MY&APPID=dbd7235bcc77e5896c73e975d013debe

dbd7235bcc77e5896c73e975d013debe
*/

void loop()
{
    // Send an HTTP GET request
  if ((millis() - lastTime) > interval) {
    // Check WiFi connection status
    if(WiFi.status()== WL_CONNECTED){ 
      on_grn();    
      on_onboard_led();
      greenOn();
      String serverPath = "http://api.openweathermap.org/data/2.5/weather?q=" + city + "," + countryCode + "&APPID=" + openWeatherMapApiKey;
      
      jsonBuffer = httpGETRequest(serverPath.c_str());
      #ifdef DEBUG_SERIAL
      Serial.println(jsonBuffer);
      #endif
      JSONVar myObject = JSON.parse(jsonBuffer);
  
      // JSON.typeof(jsonVar) can be used to get the type of the var
      if (JSON.typeof(myObject) == "undefined") {
        #ifdef DEBUG_SERIAL
        Serial.println("Parsing input failed!");
        #endif
        return;
      }

      // 0K − 273.15

      String jsonString = JSON.stringify(myObject["main"]["temp"]);
      float tempK = jsonString.toFloat(); 
      float tempC = tempK - 273.15;
    
      #ifdef DEBUG_SERIAL
      Serial.print("JSON object = ");
      Serial.println(myObject);
      Serial.print("Temperature: ");
      Serial.println(tempC);
      Serial.print("Pressure: ");
      Serial.println(myObject["main"]["pressure"]);
      Serial.print("Humidity: ");
      Serial.println(myObject["main"]["humidity"]);
      Serial.print("Wind Speed: ");
      Serial.println(myObject["wind"]["speed"]);
      Serial.print("Weather: ");
      Serial.println(myObject["weather"][0]["description"]);
      Serial.print("Feels like: ");
      Serial.println(myObject["main"]["feels_like"]);
      #endif
    }
    else {
      #ifdef DEBUG_SERIAL
      Serial.println("WiFi Disconnected");
      #endif
      on_blu();
      blueOn();
      delay(1000);
      ESP.restart();
    }
    lastTime = millis();
  }
  off_rgb();
  rgbOff();
  off_onboard_led();
} // end of void loop

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
    Serial.print("Error code: ");
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