#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <Arduino_JSON.h>

WiFiClientSecure  client_https;
WiFiClient        client_http;

const int   httpPort    = 443; // 443
const char* server_geo  = "api.geoapify.com";  // Server URL (basic) without https/http
const char* server_owm  = "api.openweathermap.org";  // Server URL (basic) without https/http
// const char*  server_google = "maps.googleapis.com";  // Server URL (basic) without https/http

float lat = 1.4471437206974116;
float lon = 110.45102315640258;

float lat1 = 1.4471437206974116;
float lon1 = 110.45102315640258;

float lat2 = 1.4560609713111177;
float lon2 = 110.42437033120086;

float lat3 = 1.543127818033014; 
float lon3 = 110.33950758881247; 

String city = "Kuching";
String countryCode = "MY";

// char ssid[] = "UiTM WiFi IoT";
// char pass[] = ""; // leave this empty as this is an open network

char ssid[] = "MaxisONE Fibre 2.4G";
char pass[] = "respironics"; 
void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);

  WiFi.begin(ssid,pass);
  Serial.print("\nConnecting to ");
  Serial.println(ssid);

  while(WiFi.status() != WL_CONNECTED){
      Serial.print("|");
      delay(10);
  }

  Serial.println("\nConnected to the WiFi network");
  Serial.print("Local ESP32 IP: ");
  Serial.println(WiFi.localIP());   
  
  client_https.setInsecure(); // dedicated for Google Maps API or GeoApify | _20 Feb 2023 Success

  // get_address(lat1,lon1);
  // get_address(lat2,lon2);
  String add_iq = get_address(lat3,lon3);
  String add1 = get_address(lat3,lon3);
  String weatherKch = get_weather("Kuching", countryCode);
  String weatherSibu = get_weather("Sibu", countryCode);
  Serial.print("Address 1 (LocationIQ):"); Serial.println(add_iq);
  Serial.print("Address 1:"); Serial.println(add1);

  Serial.print("Weather in Kuching:"); Serial.println(weatherKch);
  Serial.print("Weather in Sibu:"); Serial.println(weatherSibu);

  const char* svr = "httpbin.org";
  const char* url = "https://httpbin.org/get";
  Serial.println(parse_json_httpbin(https_request(svr, url)));

} // end of setup

void loop() {
  // put your main code here, to run repeatedly:

} // end of loop

void parse_json(String buff){
  JSONVar data = JSON.parse(buff);
  if (JSON.typeof(data) == "undefined") {
    Serial.println("Parsing input failed!");
    return;
  }
  Serial.println("Parsing input successful!");
}

String parse_json_httpbin(String buff){
  JSONVar data = JSON.parse(buff);
  if (JSON.typeof(data) == "undefined") {
    Serial.println("Parsing input failed!");
    return "Parsing input failed!";
  }
  Serial.println("Parsing input successful!");
  return JSON.stringify(data["origin"]);
}

String parse_json_weather(String buff){
  JSONVar data = JSON.parse(buff);
  if (JSON.typeof(data) == "undefined") {
    Serial.println("Parsing input failed!");
    return "Parsing input failed!";
  }
  // Serial.println("Parsing input successful!");
  // String name = JSON.stringify(data["name"]);
  return JSON.stringify(data["weather"][0]["description"]);
}

String get_weather(String city, String countryCode){
  String openWeatherMapApiKey = "dbd7235bcc77e5896c73e975d013debe";
  String serverOpenWeather = "http://api.openweathermap.org/data/2.5/weather?q=" + city + "," + countryCode + "&APPID=" + openWeatherMapApiKey;
  const char* url = serverOpenWeather.c_str();
  String out = http_request(server_owm, url); 
  return parse_json_weather(out); 
}

String parse_json_geoapi(String buff){
  JSONVar data = JSON.parse(buff);
  if (JSON.typeof(data) == "undefined") {
    Serial.println("Parsing input failed!");
    return "Parsing input failed!";
  }
  // Serial.println("Parsing input successful!");
  return JSON.stringify(data["features"][0]["properties"]["address_line1"]);
}

String get_address(float lat, float lon){
  String urlPiece = "/v1/geocode/reverse?lat="+String(lat)+"&lon="+String(lon);
  String urlKey = "aad49482771c41c8bd927acac874e28a";
  String serverGeoapify = "https://" + String(server_geo) + urlPiece + "&apiKey=" + urlKey;
  const char* url = serverGeoapify.c_str();
  String out = https_request(server_geo, url);
  return parse_json_geoapi(out);
}

String get_address_iq(float lat, float lon){
  String locationiq_key = "pk.b82fc9de1d0f74c8d99cca7292290fe2";
  String url_iq = "https://us1.locationiq.com/v1/reverse?key="+locationiq_key+"&lat="+String(lat)+"&lon="+String(lon)+"&format=json";
  const char* url = url_iq.c_str();
  String out = https_request("us1.locationiq.com", url);
  return parse_json_locationiq(out);
}

String parse_json_locationiq(String buff){
  JSONVar data = JSON.parse(buff);
  if (JSON.typeof(data) == "undefined") {
    Serial.println("Parsing input failed!");
    return "Parsing input failed!";
  }
  // Serial.println("Parsing input successful!");
  return JSON.stringify(data["display_name"]);
}

String http_request(const char* server, const char* url){
  while (!client_http.connect(server, httpPort)) {
    Serial.printf("[ERROR]: Connection to %s failed!\n", server);
    Serial.print("Retrying connecting");
    for(int i=0; i<50; i++){
      Serial.print(".");
      delay(100);
    }
  }
  Serial.printf("\n[SUCCESS]: Connected to server: %s \n",server);
  Serial.print("\nSending HTTP Request.\n");
  String jsonBuffer = http_GET_request(url);
  // Serial.printf("[OpenWeather] HTTP payload out is \n");
  // Serial.println(jsonBuffer);
  // Serial.printf("[End of HTTP payload]\n");
  return jsonBuffer;

}

String https_request(const char* server, const char* url){

  while (!client_https.connect(server, httpPort)) {
    Serial.printf("[ERROR]: Connection to %s failed!\n", server);
    Serial.print("Retrying connecting");
    for(int i=0; i<50; i++){
      Serial.print(".");
      delay(100);
    }
  }
  Serial.printf("\n[SUCCESS]: Connected to server: %s \n",server);
  Serial.print("\nSending HTTPS Request.\n");
  String jsonBufferGeo = https_GET_request(url);
  // Serial.printf("HTTPS payload out is \n");
  // Serial.println(jsonBufferGeo);
  // Serial.printf("[End of HTTPS payload]\n");
  return jsonBufferGeo;
}


String https_GET_request(const char* serverName) {
  HTTPClient https; // create an instance to represent https
  String payload = "{None}";
  // Your Domain name with URL path or IP address with path
  https.begin(client_https, serverName);
  
  // Send HTTP POST request
  int httpCode = https.GET();
  
  if (httpCode > 0) {
    Serial.print("HTTPS response code: ");
    Serial.println(httpCode);
    if(httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) 
      payload = https.getString();
  }
  else {
    Serial.print("HTTPS GET Error code: ");
    Serial.println(httpCode);
    Serial.printf("[HTTPS] GET... failed, error: %s\n", https.errorToString(httpCode).c_str());

  }
  // Free resources
  https.end();

  return payload;
}

String http_GET_request(const char* serverName) {
  HTTPClient http; // insecure 

  String payload = "{None}";
    
  // Your Domain name with URL path or IP address with path
  http.begin(client_http, serverName);
  
  // Send HTTP POST request
  int httpCode = http.GET(); 
  
  if (httpCode>0) {
    Serial.print("HTTP response code: ");
    Serial.println(httpCode);
    if(httpCode == HTTP_CODE_OK) 
      payload = http.getString();
  }
  else {
    Serial.print("HTTP GET Error code: ");
    Serial.println(httpCode);
  }
  // Free resources
  http.end();

  return payload;
}

