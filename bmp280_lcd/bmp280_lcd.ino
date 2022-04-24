/***************************************************************************
  This is a library for the BMP280 humidity, temperature & pressure sensor

  Designed specifically to work with the Adafruit BMP280 Breakout
  ----> http://www.adafruit.com/products/2651

  These sensors use I2C or SPI to communicate, 2 or 4 pins are required
  to interface.

  Adafruit invests time and resources providing this open source code,
  please support Adafruit andopen-source hardware by purchasing products
  from Adafruit!

  Written by Limor Fried & Kevin Townsend for Adafruit Industries.
  BSD license, all text above must be included in any redistribution
 ***************************************************************************/

#include <Wire.h>
#include <SPI.h>
#include <Adafruit_BMP280.h>

#define LCD_AVAILABLE

#ifdef LCD_AVAILABLE
//  #include "rgb_lcd.h"
//  rgb_lcd        lcd;
  #include <Wire.h> 
  #include <LiquidCrystal_I2C.h>
  LiquidCrystal_I2C lcd(0x27,16,2);  // set the LCD address to 0x27 for a 16 chars and 2 line display
#endif

#define BMP_SCK  (13)
#define BMP_MISO (12)
#define BMP_MOSI (11)
#define BMP_CS   (10)

Adafruit_BMP280 bmp; // I2C
//Adafruit_BMP280 bmp(BMP_CS); // hardware SPI
//Adafruit_BMP280 bmp(BMP_CS, BMP_MOSI, BMP_MISO,  BMP_SCK);

void setup() {
  Serial.begin(115200);

   #ifdef LCD_AVAILABLE
//    lcd.begin(16, 2); // Grove Blue Backlit LCD
    lcd.init();         // Normal i2c LCD
    lcd.backlight();    // Normal i2c LCD
    lcd.setCursor(0, 0); // row 1, column 0
    lcd.print("BMP280 Sensor"); // MAKE SURE THIS IS CHECKED BEFORE UPLOAD  
    lcd.setCursor(0, 1); // row 1, column 0
    lcd.print("Starting Node");
  #endif
  Serial.println(F("BMP280 test"));

  if (!bmp.begin(0x76)) {
    Serial.println(F("Could not find a valid BMP280 sensor, check wiring!"));
    while (1);
  }

  /* Default settings from datasheet. */
  bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,     /* Operating Mode. */
                  Adafruit_BMP280::SAMPLING_X2,     /* Temp. oversampling */
                  Adafruit_BMP280::SAMPLING_X16,    /* Pressure oversampling */
                  Adafruit_BMP280::FILTER_X16,      /* Filtering. */
                  Adafruit_BMP280::STANDBY_MS_500); /* Standby time. */
}

void loop() {
    int tempc = bmp.readTemperature();
    Serial.print(F("Temperature = "));
    Serial.print(tempc);
    Serial.println(" *C");

    float pres_hpa = bmp.readPressure();
    Serial.print(F("Pressure = "));
    Serial.print(pres_hpa);
    Serial.println(" Pa");

    int alt = bmp.readAltitude(1013.25);
    Serial.print(F("Approx altitude = "));
    Serial.print(alt); /* Adjusted to local forecast! */
    Serial.println(" m");

    Serial.println();
    #ifdef LCD_AVAILABLE
    lcd.clear();
    lcd.setCursor(0, 0); // row 1, column 0
    lcd.print("T:" +String(tempc)+"*C P:" +String(pres_hpa)+" hPa");
    lcd.setCursor(0, 1); // row 1, column 0
    lcd.print("Altitude:" +String(alt)+" m");
    #endif
    delay(2000);
}
