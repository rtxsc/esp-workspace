// #define ENABLE_OLED
#define ENABLE_LCD
#include <Wire.h>
#include "rgb_lcd.h"
#include <TinyGPS++.h>
#include <SoftwareSerial.h>

#define PARAMETER_COUNT 4

#ifdef ENABLE_LCD
rgb_lcd lcd;
#endif

#ifdef ENABLE_OLED
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET     1 

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
#endif
/*
   This sample code demonstrates how to use an array of TinyGPSCustom objects
   to monitor all the visible satellites.

   Satellite numbers, elevation, azimuth, and signal-to-noise ratio are not
   normally tracked by TinyGPS++, but by using TinyGPSCustom we get around this.

   The simple code also demonstrates how to use arrays of TinyGPSCustom objects,
   each monitoring a different field of the $GPGSV sentence.

   It requires the use of SoftwareSerial, and assumes that you have a
   4800-baud serial GPS device hooked up on pins 4(RX) and 3(TX).
*/
static const int RXPin = 6, TXPin = 7;
static const uint16_t GPSBaud = 9600;
// The TinyGPS++ object
TinyGPSPlus gps;

// The serial connection to the GPS device
SoftwareSerial ss(RXPin, TXPin);

/* 
  From http://aprs.gids.nl/nmea/:
   
  $GPGSV
  
  GPS Satellites in view
  
  eg. $GPGSV,3,1,11,03,03,111,00,04,15,270,00,06,01,010,00,13,06,292,00*74
      $GPGSV,3,2,11,14,25,170,00,16,57,208,39,18,67,296,40,19,40,246,00*74
      $GPGSV,3,3,11,22,42,067,42,24,14,311,43,27,05,244,00,,,,*4D

  1    = Total number of messages of this type in this cycle
  2    = Message number
  3    = Total number of SVs in view
  4    = SV PRN number
  5    = Elevation in degrees, 90 maximum
  6    = Azimuth, degrees from true north, 000 to 359
  7    = SNR, 00-99 dB (null when not tracking)
  8-11 = Information about second SV, same as field 4-7
  12-15= Information about third SV, same as field 4-7
  16-19= Information about fourth SV, same as field 4-7
*/

static const int MAX_SATELLITES = 40;
static const int GMT_OFFSET     = 8; // GMT+8

TinyGPSCustom totalGPGSVMessages(gps, "GPGSV", 1); // $GPGSV sentence, first element
TinyGPSCustom messageNumber(gps, "GPGSV", 2);      // $GPGSV sentence, second element
TinyGPSCustom satsInView(gps, "GPGSV", 3);         // $GPGSV sentence, third element
TinyGPSCustom satNumber[PARAMETER_COUNT]; // to be initialized later
// TinyGPSCustom elevation[4];
// TinyGPSCustom azimuth[4];
TinyGPSCustom snr[PARAMETER_COUNT];



void setup()
{
  #ifdef ENABLE_LCD
  lcd.begin(16, 2);
  lcd.setCursor(0,0);
  lcd.print("Satellite Track!");
  lcd.setCursor(0,1);
  lcd.print("-by  ClumzyaziD-");
  delay(1000);
  #endif

  #ifdef ENABLE_OLED
  Wire.begin();
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.setTextColor(WHITE);
  display.println(F("Satellite Tracker!"));
  display.display();
  delay(2000); // Pause for 2 seconds 
  #endif
  Serial.begin(115200);
  ss.begin(GPSBaud);

  Serial.println(F("SatelliteTracker.ino modified by RTXSC"));
  Serial.println(F("Monitoring satellite location and signal strength using TinyGPSCustom"));
  Serial.println();
  
  // Initialize all the uninitialized TinyGPSCustom objects
  // satNumber[0].begin(gps, "GPGSV", 4 + 4 * 0); // offsets 4, 8, 12, 16
  // snr[0].begin(gps, "GPGSV", 7 + 4 * 0); // offsets 7, 11, 15, 19
  for (int i=0; i<PARAMETER_COUNT; ++i) // originally condition i<4
  {
    satNumber[i].begin(gps, "GPGSV", 4 + 4 * i); // offsets 4, 8, 12, 16
    // elevation[i].begin(gps, "GPGSV", 5 + 4 * i); // offsets 5, 9, 13, 17
    // azimuth[i].begin(  gps, "GPGSV", 6 + 4 * i); // offsets 6, 10, 14, 18
    snr[i].begin(gps, "GPGSV", 7 + 4 * i); // offsets 7, 11, 15, 19
  }
}

  // struct
  // {
  //   bool active;
  //   // int elevation;
  //   // int azimuth;
  //   // int snr;
  // } sats[MAX_SATELLITES];


void loop()
{
  #ifdef ENABLE_OLED
  display.clearDisplay();
  #endif
  // Dispatch incoming characters
  if (ss.available() > 0)
  {
    uint8_t active_satellite;
    int snr_val;
    gps.encode(ss.read());

  if (totalGPGSVMessages.isUpdated()){
      #ifdef ENABLE_LCD
      lcd.clear();
      #endif
      int totalMessages = atoi(totalGPGSVMessages.value());
      int currentMessage = atoi(messageNumber.value());
      int currentCharsInt = gps.charsProcessed()/162;
      Serial.print("Total Msg:"); Serial.print(totalMessages);
      Serial.print("  Current Msg:"); Serial.print(currentMessage);
      Serial.print("  Current Chars:"); Serial.println(currentCharsInt);

      if (totalMessages == currentMessage){
          Serial.println("\n---------------Total GPGSV Messages updated!---------------");
          Serial.print("[Self] Sats In Use:"); Serial.println(gps.satellites.value());
          for (int i=0; i<PARAMETER_COUNT; ++i) // originally condition i<4
          {
            active_satellite = atoi(satNumber[i].value());
            snr_val          = atoi(snr[i].value());;
            Serial.print("Sat Number ["); Serial.print(i); Serial.println("]");
            Serial.print(F("[Self] Active Sat Number is ")); Serial.println(active_satellite);
            Serial.print(F("[Self] SNR is (dB) ")); Serial.println(snr_val);   
          }
      }
      #ifdef ENABLE_LCD 
      if((millis()/1000) % 2  == 0){
        lcd.setCursor(0,0);
        lcd.print("Use:"+ String(gps.satellites.value()));
        lcd.setCursor(6,0);
        lcd.print(" C:"+ String(currentCharsInt));
        lcd.setCursor(0,1);
        lcd.print("Active Sat#:"+ String(active_satellite));
      }else{
        lcd.setCursor(0,0);
        lcd.print("SatTime:"+ TimePrint());
        lcd.setCursor(0,1);
        lcd.print("S-N-R: "+ String(snr_val) + " dB");
      }
      
      #endif
      delay(1000); // internal delay
      Serial.println();
  }

    // if (totalGPGSVMessages.isUpdated())
    // {
      // for (int i=0; i<PARAMETER_COUNT; ++i)
      // {
      //   int no = atoi(satNumber[i].value());
      //   active_satellite = no;
      //   Serial.print(F("Active Sat Number is ")); Serial.println(no);
      //   // if (no >= 1 && no <= MAX_SATELLITES)
      //   // {
      //   //   // sats[no-1].elevation = atoi(elevation[i].value());
      //   //   // sats[no-1].azimuth = atoi(azimuth[i].value());
      //   //   // sats[no-1].snr = atoi(snr[i].value());
      //   //   sats[no-1].active = true;
      //   // }
      // }
      
      // int totalMessages = atoi(totalGPGSVMessages.value());
      // int currentMessage = atoi(messageNumber.value());
      // if (totalMessages == currentMessage)
      // {
        // TimePrint();
        // Serial.print(F("Total Msg:")); 
        // Serial.print(totalMessages);
        // Serial.print(F(" Sats In Use=")); Serial.print(gps.satellites.value());
        // Serial.print(F(" Sats Active="));
        // Serial.print(active_satellite);

        #ifdef ENABLE_OLED
        display.setCursor(0,0);             // Start at top-left corner
        display.print(F("Sats="));
        display.println(gps.satellites.value());
        display.setCursor(0,8);             // Start at top-left corner
        display.print(F("Active="));
        display.println(active_satellite);
        display.display();
        #endif

        // int elevation_val;
        // int azimuth_val;
        // int snr_val;

        // for (int i=0; i<MAX_SATELLITES; ++i)
        //   if (sats[i].active)
        //   {
        //     active_satellite = i+1;
        //     Serial.print(i+1);
        //     Serial.print(F(" "));
        //   }

        // Serial.print(F(" Elevation="));
        // for (int i=0; i<MAX_SATELLITES; ++i)
        //   if (sats[i].active)
        //   {
        //     elevation_val = sats[i].elevation;
        //     Serial.print(sats[i].elevation);
        //     Serial.print(F(" "));
        //   }
        // Serial.print(F(" Azimuth="));
        // for (int i=0; i<MAX_SATELLITES; ++i)
        //   if (sats[i].active)
        //   {
        //     azimuth_val = sats[i].azimuth;
        //     Serial.print(sats[i].azimuth);
        //     Serial.print(F(" "));
        //   }
        
        // Serial.print(F(" SNR="));
        // for (int i=0; i<MAX_SATELLITES; ++i)
        //   if (sats[i].active)
        //   {
        //     snr_val = sats[i].snr;
        //     Serial.print(sats[i].snr);
        //     Serial.print(F(" "));
        //   }
        // Serial.println();

        // for (int i=0; i<MAX_SATELLITES; ++i)
        //   sats[i].active = false;
        // delay(1000); // master delay
      // } // end of if (totalMessages == currentMessage)
    // } // end of totalGPGSVMessages
  }
}

String TimePrint()
{
  if (gps.time.isValid())
  {
    String h,m,s;
    uint8_t hour = gps.time.hour() + GMT_OFFSET;
    if (hour < 10) h = "0" + String(hour);
    else h = String(hour);

    if (gps.time.minute() < 10) m = "0" + String(gps.time.minute());
    else m = String(gps.time.minute());

    if (gps.time.second() < 10) s = "0" + String(gps.time.second());
    else s = String(gps.time.second());
    String time_f = h + ":" + m + ":" + s;
                
    // Serial.print("Time formatted:");
    // Serial.println(time_f);

    // if (gps.time.hour() < 10)
    //   Serial.print(F("0"));
    // Serial.print(gps.time.hour());
    // Serial.print(F(":"));
    // if (gps.time.minute() < 10)
    //   Serial.print(F("0"));
    // Serial.print(gps.time.minute());
    // Serial.print(F(":"));
    // if (gps.time.second() < 10)
    //   Serial.print(F("0"));
    // Serial.print(gps.time.second());
    // Serial.print(F(" "));
    return time_f;
  }
  else
  {
    Serial.print(F("(unknown)"));
    return "HH:MM:SS";
  }
}