#include <WiFiClientSecure.h>
#include <ArduinoJson.h>

// #define GPS_AVAILABLE

#define uS_TO_S_FACTOR 1000000  //Convert uSecs to Secs
#define TIME_TO_SLEEP  60       //How long should the ESP deepsleep ?
RTC_DATA_ATTR int bootCount = 0;

// Enter your Wifi Info
const char* ssid     = "NPRDC CELCOM M2";
const char* password = "nprdc1234";

const char* host = "https://maps.googleapis.com"; 

// Char arrays to hold final values for request
static char LAT[10];
static char LONG[10];
char veribuff[2];

//Declare the Hardware Serial to be used by the GPS
HardwareSerial GPSSerial(2);

//For Holding the GPS Serial Data
unsigned char buffer[256];
int count = 0;


void setup() {
  //Deepsleep setup
  ++bootCount;
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);

  //Start the hardware serial (rx2,tx2) to read data from gps device
  //Initialize serial monitor and wait for port to open
  GPSSerial.begin(9600, SERIAL_8N1, 16, 17);
  Serial.begin(115200);
  delay(10);
  Serial.print("#################\nInitializing...");

  //Connect to Wifi
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }


  //Display on Serial Monitor
  Serial.println("\n#################");
  Serial.print("IP address : ");
  Serial.println(WiFi.localIP());
  Serial.println("Serials are Connected.");
  Serial.println("Waiting For GPS Data...");
  Serial.println("#################");

}
//End void setup()

void loop() {
  #ifdef GPS_AVAILABLE
  //Read NMEA Data from GPS device connected to Serial
  while (GPSSerial.available()) {
    buffer[count++] = GPSSerial.read();
    if (count == 256) {
      break;
    }
  }

  ///////////////////////////////////////////////
  // The section below deals with getting the data 
  //into a String and slicing out the values needed
  ///////////////////////////////////////////////

  //Cast GPS Data Buffer as String
  String myString = (const char*)buffer;

  //Find the Line you are interested in
  if (myString.startsWith("$GPRMC")) {

    //Trim String from second "$" to end
    myString.remove(myString.indexOf("$", 1));

    //Trim String up to first ","
    int idx = myString.indexOf(",");
    idx++;
    myString.remove(0, idx);

    //Trim String up to first ","
    idx = myString.indexOf(",");
    idx++;
    myString.remove(0, idx);

    //Now String begins with Verification Value
   
    myString.toCharArray(veribuff, 2);
    Serial.print("Receiver Verification : ");
    Serial.println(veribuff);

    //Trim String up to first ","
    idx = myString.indexOf(",");
    idx++;
    myString.remove(0, idx);

    //Now String begins with Latitude Value
    char latibuff[11];
    char outLat[2];
    char calcbuf[9];
    myString.toCharArray(latibuff, 11);

    //Extract the Degree Section
    outLat[0] = latibuff[0];
    outLat[1] = latibuff[1];

    //Extract The Minutes and Seconds for Decimalization
    for (int i = 0; i <= 8; i++) {
      calcbuf[i] = latibuff[i + 2];
    }

    float calcVal;
    calcVal = atof(outLat) + (atof(calcbuf) / 60);


    //Moved Serial Prints below to account for +- NS


    //Trim String up to first ","
    idx = myString.indexOf(",");
    idx++;
    myString.remove(0, idx);

    //Now String begins with N/S Value (+-)
    char NSbuff[2];
    myString.toCharArray(NSbuff, 2);

    //To Account for +- NS affecting the sign of Latitude
    //Southern Hemisphere Latitudes have a minus sign
    if (NSbuff[0] == 'S') {
      calcVal *= -1;
    }

    Serial.print("Latitude : ");
    Serial.println(calcVal, 5);

    Serial.print("N/S : ");
    Serial.println(NSbuff[0]);

    //Trim String up to first ","
    idx = myString.indexOf(",");
    idx++;
    myString.remove(0, idx);

    //Now String begins with Longitude Value
    char longibuff[12];
    char outLNG[3];
    char calcbuf2[9];
    myString.toCharArray(longibuff, 12);

    //Extract the Degree Section
    outLNG[0] = longibuff[0];
    outLNG[1] = longibuff[1];
    outLNG[2] = longibuff[2];

    //Extract The Minutes and Seconds for Decimalization
    for (int i = 0; i <= 8; i++) {
      calcbuf2[i] = longibuff[i + 3];
    }

    float calcVal2;

    calcVal2 = atof(outLNG) + (atof(calcbuf2) / 60);


    // Moved Serial prints below to account for +- EW

    //Trim String up to first ","
    idx = myString.indexOf(",");
    idx++;
    myString.remove(0, idx);

    //Now String begins with E/W Value (+-)
    char EWbuff[2];
    myString.toCharArray(EWbuff, 2);

    //To Account for +- EW which affects the sign of Longitude
    //Western Hemisphere Latitudes have a minus sign
    if (EWbuff[0] == 'W') {
      calcVal2 *= -1;
    }
    
    Serial.print("Longitude : ");
    Serial.println(calcVal2, 5);

    Serial.print("E/W : ");
    Serial.println(EWbuff[0]);
    Serial.println("#################");

    //Convert the Floats, Latitude & Longtitude to char[] for sending in Request
    dtostrf(calcVal, 7, 5, LAT);
    dtostrf(calcVal2, 7, 5, LONG);

    #endif

    float dummyLat = 1.4491; // dummy 1.4491, 110.4490
    float dummyLon = 110.4490; // dummy
    veribuff[0] = 'A'; // dummy
    dtostrf(dummyLat, 7, 5, LAT);
    dtostrf(dummyLon, 7, 5, LONG);


    ///////////////////////////////////////////////
    // The section below deals with connection
    // to google maps API and making a GET Request
    ///////////////////////////////////////////////

    /*
    https://maps.googleapis.com/maps/api/geocode/json?latlng=1.4491,110.4490&key=AIzaSyBn9oZIwFaL3hpwu4nr1FhCYEomRI1kzFE
    */
    //Check to see if Receiver is to be trusted before making request
    if (veribuff[0] == 'A') {
      // WiFiClientSecure client;
      WiFiClient client;

      const int httpPort = 443;
      if (!client.connect(host, httpPort)) {
        Serial.println("Error: Connection to Google Failed !!!");
        Serial.println("Waiting 2 secs");
        delay(2000);
        return;
      }
      
      Serial.println("Connected to Google Maps!!!");

      // VERY IMPORTANT !!!!
      // String urlKey, just below is where you add your API key 
      // Note the "&key= " at the beginning.

      //Building the GET request
      String urlPiece = "/maps/api/geocode/json?latlng=";
      String urlKey = "&key=AIzaSyBn9oZIwFaL3hpwu4nr1FhCYEomRI1kzFE";
      String url = urlPiece + LAT + ',' + LONG + urlKey;

      Serial.print("\nSending Request.");
      Serial.println(url + "\n");

      //Send request
      client.print(String("GET ") + url + "\r\n" +
                   "Host: " + host + "\r\n" +
                   "Connection: close\r\n\r\n");

      unsigned long timeout = millis();

      while (client.available() == 0) {
        if (millis() - timeout > 5000) {
          Serial.println("Error: Client Timeout !!!");
          client.stop();
          return;
        }
      }

      //Check HTTP status
      char status[32] = {0};
      client.readBytesUntil('\r', status, sizeof(status));
      if (strcmp(status, "HTTP/1.0 200 OK") != 0) {
        Serial.print(F("Unexpected response: "));
        Serial.println(status);
        return;
      }

      //Skip HTTP headers
      char endOfHeaders[] = "\r\n\r\n";
      if (!client.find(endOfHeaders)) {
        Serial.println(F("Invalid response"));
        return;
      }

      //Use arduinojson.org/assistant to compute the capacity.
      const size_t bufferSize = JSON_ARRAY_SIZE(1) + 2 * JSON_OBJECT_SIZE(2) + JSON_OBJECT_SIZE(3) + 120;

      //Allocate JsonBuffer
      DynamicJsonBuffer jsonBuffer(bufferSize);

      //Parse JSON object
      JsonObject& root = jsonBuffer.parseObject(client);
      if (!root.success()) {
        Serial.println(F("Parsing failed!"));
        return;
      }

      JsonObject& results0 = root["results"][0];
      String results0_formatted_address = results0["formatted_address"];
      Serial.println("#################");
      Serial.println(results0_formatted_address);
      Serial.println("#################\n");
      Serial.print("Address Returned, going to sleep for ");
      Serial.print(TIME_TO_SLEEP);
      Serial.println(" Secs....");
      Serial.flush();
      GPSSerial.flush();
      esp_deep_sleep_start();
    }

  #ifdef GPS_AVAILABLE
    } // part of entire GPS structure! DO NOT DELETE
  #endif
  bufferClear();

}
//End void loop()


void bufferClear()
{
  for (int i = 0; i < count; i++)
  {
    buffer[i] = NULL;
    count = 0;
  }
}