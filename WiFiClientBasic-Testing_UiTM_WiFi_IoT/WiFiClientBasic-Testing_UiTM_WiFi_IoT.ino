#include <Wire.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <Arduino_JSON.h>
#include "uptime_formatter.h"
#include "uptime.h"
#define ESP32S2

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

// #define DEBUG_SERIAL
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

String jsonBuffer   = "None";
String jsonString   = "None";
String jsonHumid    = "None";
String jsonWeather  = "None";

float tempK ; 
float tempC ;
float humid ; 

void setup(){
    #ifdef DEBUG_SERIAL
    Serial.begin(115200);
    #endif
    // Wire.begin(I2C_SDA, I2C_SCL);
    Wire.begin();

    pinMode(ONBOARD_LED, OUTPUT);
    pinMode(RED, OUTPUT);
    pinMode(GRN, OUTPUT);
    pinMode(BLU, OUTPUT);

    lcd.begin(16, 2);
    lcd.setCursor(0,0);
    lcd.print("Hello from ESP32S2");
    lcd.setCursor(0,1);
    lcd.print("Connecting WiFi");
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
  int RSSI_dBm =  WiFi.RSSI();
  String rssi_state = get_rssi_state(RSSI_dBm);
  display_uptime_top_row();
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
      // Serial.println(jsonBuffer); // print all JSON output
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

      jsonString = JSON.stringify(myObject["main"]["temp"]);
      jsonHumid = JSON.stringify(myObject["main"]["humidity"]);

      tempK = jsonString.toFloat(); 
      tempC = tempK - 273.15;
      humid = jsonHumid.toFloat(); 

      jsonWeather = JSON.stringify(myObject["weather"][0]["description"]);
      // String jsonWeather = "thunderstorm with light rain";
  
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
      lcd.clear();
      lcd.setCursor(0,0); // row 0, column 0
      lcd.print("WiFi Disconnect"); // print connected SSID
      lcd.setCursor(0,1); // row 0, column 0
      lcd.print("Restart ESP Now"); // print connected SSID
      on_blu();
      blueOn();
      delay(1000);
      ESP.restart();
    }
    lastTime = millis();
  }
  // taken out from if(WiFi.status()== WL_CONNECTED) logic 20 Jan 2023
  // solving the weird frozen issue when i2c LCD attached 
  if(millis() % 2 == 0){
        lcd.setCursor(0,1);
        lcd.print(jsonWeather);
        // int str_length = jsonWeather.length();
        // if(str_length > 16){
        //   for (int positionCounter = 0; positionCounter < 13; positionCounter++) {
        //     // scroll one position left:
        //     lcd.scrollDisplayLeft();
        //     // wait a bit:
        //     delay(150);
        //   }
        // }
      }
      else if(millis() % 5 == 0 || millis() % 7 == 0){
        lcd.clear();
        lcd.print("RSSI:" + String(RSSI_dBm) + " dBm");
        lcd.setCursor(0,1); // row 0, column 0
        lcd.print(ssid); // print connected SSID
      }
      else{
        lcd.clear();
        lcd.print(rssi_state);
        lcd.setCursor(0,1);
        lcd.print(String(tempC) + " " + String(humid)+"%");        
      }
  
  off_rgb();
  rgbOff();
  off_onboard_led();
  delay(1000);

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