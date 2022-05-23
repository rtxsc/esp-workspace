#include <TinyGPS++.h>
#include <SoftwareSerial.h>
#include "GroveBase-ESPDuino32-Mapping.h"


#define LCD_AVAILABLE
//#define GROVE_LCD // uncomment this if using GROVE LCD

#ifdef LCD_AVAILABLE
  #ifdef GROVE_LCD
    #include "rgb_lcd.h"
    rgb_lcd        lcd;
  #else
    #include <Wire.h> 
    #include <LiquidCrystal_I2C.h>
    LiquidCrystal_I2C lcd(0x27,16,2); 
  #endif
#endif
static const int RXPin = 6, TXPin = 7; // D6->TXgps D7->RXgps | GROVE_D6 / GROVE_D7 on ESP32
//static const int RXPin = GROVE_D6, TXPin = GROVE_D7; // D6->TXgps D7->RXgps | GROVE_D6 / GROVE_D7 on ESP32
static const uint32_t GPSBaud = 9600;
int currentCharsInt = 0;
int prevCharsInt = 0;

// The TinyGPS++ object
TinyGPSPlus gps;

// The serial connection to the GPS device
SoftwareSerial ss(RXPin, TXPin);

void setup()
{
  Serial.begin(115200);

    #ifdef LCD_AVAILABLE
      #ifdef GROVE_LCD
        lcd.begin(16, 2); // Grove Blue Backlit LCD
      #else
        lcd.init();         // Normal i2c LCD
        lcd.backlight();    // Normal i2c LCD
    #endif
    lcd.setCursor(0, 0); // row 1, column 0
    lcd.print("IoT Node GPS"); // MAKE SURE THIS IS CHECKED BEFORE UPLOAD  
    lcd.setCursor(0, 1); // row 1, column 0
    lcd.print("Starting Node");
  #endif
  ss.begin(GPSBaud);

  Serial.println(F("FullExample.ino"));
  Serial.println(F("An extensive example of many interesting TinyGPS++ features"));
  Serial.print(F("Testing TinyGPS++ library v. ")); Serial.println(TinyGPSPlus::libraryVersion());
  Serial.println(F("by Mikal Hart"));
  Serial.println();
  Serial.println(F("Sats HDOP  Latitude   Longitude   Fix  Date       Time     Date Alt    Course Speed Card  Distance Course Card  Chars Sentences Checksum"));
  Serial.println(F("           (deg)      (deg)       Age                      Age  (m)    --- from GPS ----  ---- to London  ----  RX    RX        Fail"));
  Serial.println(F("----------------------------------------------------------------------------------------------------------------------------------------"));
}

void loop()
{ 
//  static const double LONDON_LAT = 51.508131, LONDON_LON = -0.128002;
//  Serial.println(F("Sats HDOP  Latitude   Longitude   Fix  Date       Time     Date Alt    Course Speed Card  Distance Course Card  Chars Sentences Checksum"));
//  Serial.println(F("           (deg)      (deg)       Age                      Age  (m)    --- from GPS ----  ---- to London  ----  RX    RX        Fail"));
//  Serial.println(F("----------------------------------------------------------------------------------------------------------------------------------------"));
//  printInt(gps.satellites.value(), gps.satellites.isValid(), 5);
//  printFloat(gps.hdop.hdop(), gps.hdop.isValid(), 6, 1);
//  printFloat(gps.location.lat(), gps.location.isValid(), 11, 6);
//  printFloat(gps.location.lng(), gps.location.isValid(), 12, 6);
//  printInt(gps.location.age(), gps.location.isValid(), 5);
//  printDateTime(gps.date, gps.time);
//  printFloat(gps.altitude.meters(), gps.altitude.isValid(), 7, 2);
//  printFloat(gps.course.deg(), gps.course.isValid(), 7, 2);
//  printFloat(gps.speed.kmph(), gps.speed.isValid(), 6, 2);
//  printStr(gps.course.isValid() ? TinyGPSPlus::cardinal(gps.course.deg()) : "*** ", 6);
//
//  unsigned long distanceKmToLondon =
//    (unsigned long)TinyGPSPlus::distanceBetween(
//      gps.location.lat(),
//      gps.location.lng(),
//      LONDON_LAT, 
//      LONDON_LON) / 1000;
//  printInt(distanceKmToLondon, gps.location.isValid(), 9);
//
//  double courseToLondon =
//    TinyGPSPlus::courseTo(
//      gps.location.lat(),
//      gps.location.lng(),
//      LONDON_LAT, 
//      LONDON_LON);
//
//  printFloat(courseToLondon, gps.location.isValid(), 7, 2);
//
//  const char *cardinalToLondon = TinyGPSPlus::cardinal(courseToLondon);
//
//  printStr(gps.location.isValid() ? cardinalToLondon : "*** ", 6);

//  printInt(gps.charsProcessed()/162, true, 6);
//  printInt(gps.sentencesWithFix(), true, 10);
//  printInt(gps.failedChecksum(), true, 9);
//  Serial.println();
  
  currentCharsInt = gps.charsProcessed()/162;
  if(prevCharsInt != currentCharsInt){
    prevCharsInt = currentCharsInt;
  }
  else{
    Serial.println(F("[MISSING] No GPS data received: check wiring"));
  }
  #ifdef LCD_AVAILABLE
  lcd.clear();
  lcd.setCursor(0, 0); // row 1, column 0
  lcd.print(String(float(gps.location.lat()),4)+ " S:"+ String(gps.satellites.value()));
  lcd.print(" "+String(gps.speed.kmph()));
  lcd.setCursor(0, 1); // row 1, column 0
  lcd.print(String(float(gps.location.lng()),4) + " C:"+ String(currentCharsInt));
  #endif
  
  smartDelay(1000);

  if (millis() > 5000 && gps.charsProcessed() < 10)
    Serial.println(F("[INITIAL] No GPS data received: check wiring"));
}

// This custom version of delay() ensures that the gps object
// is being "fed".
static void smartDelay(unsigned long ms)
{
  unsigned long start = millis();
  do 
  {
    while (ss.available())
      gps.encode(ss.read());
  } while (millis() - start < ms);
}

static void printFloat(float val, bool valid, int len, int prec)
{
  if (!valid)
  {
    while (len-- > 1)
      Serial.print('*');
    Serial.print(' ');
  }
  else
  {
    Serial.print(val, prec);
    int vi = abs((int)val);
    int flen = prec + (val < 0.0 ? 2 : 1); // . and -
    flen += vi >= 1000 ? 4 : vi >= 100 ? 3 : vi >= 10 ? 2 : 1;
    for (int i=flen; i<len; ++i)
      Serial.print(' ');
  }
  smartDelay(0);
}

static void printInt(unsigned long val, bool valid, int len)
{
  char sz[32] = "*****************";
  if (valid)
    sprintf(sz, "%ld", val);
  sz[len] = 0;
  for (int i=strlen(sz); i<len; ++i)
    sz[i] = ' ';
  if (len > 0) 
    sz[len-1] = ' ';
  Serial.print(sz);
  smartDelay(0);
}

static void printDateTime(TinyGPSDate &d, TinyGPSTime &t)
{
  if (!d.isValid())
  {
    Serial.print(F("********** "));
  }
  else
  {
    char sz[32];
    sprintf(sz, "%02d/%02d/%02d ", d.month(), d.day(), d.year());
    Serial.print(sz);
  }
  
  if (!t.isValid())
  {
    Serial.print(F("******** "));
  }
  else
  {
    char sz[32];
    sprintf(sz, "%02d:%02d:%02d ", t.hour(), t.minute(), t.second());
    Serial.print(sz);
  }

  printInt(d.age(), d.isValid(), 5);
  smartDelay(0);
}

static void printStr(const char *str, int len)
{
  int slen = strlen(str);
  for (int i=0; i<len; ++i)
    Serial.print(i<slen ? str[i] : ' ');
  smartDelay(0);
}
