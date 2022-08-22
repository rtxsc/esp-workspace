#include <Adafruit_AHT10.h>
#include <Wire.h>
#define I2C_SDA 8 // 0 for ESP01s
#define I2C_SCL 9 // 2 for ESP01s

#define CLK 18
#define DIO 19

Adafruit_AHT10 aht;

#include "GyverTM1637.h"
GyverTM1637 disp(CLK, DIO);

void setup() {
  Serial.begin(115200);
  Wire.begin(I2C_SDA, I2C_SCL);
  disp.clear();
  disp.brightness(7);

  Serial.println("Adafruit AHT10 demo!");

  if (! aht.begin()) {
    Serial.println("Could not find AHT10? Check wiring");
    while (1) delay(10);
  }
  Serial.println("AHT10 found");
}

void loop() {
  sensors_event_t humidity, temp;
  aht.getEvent(&humidity, &temp);// populate temp and humidity objects with fresh data
  Serial.print("Temperature: "); Serial.print(temp.temperature); Serial.println(" degrees C");
  Serial.print("Humidity: "); Serial.print(humidity.relative_humidity); Serial.println("% rH");
//  disp.displayInt(temp.temperature);
//  delay(1000);
//  disp.displayInt(humidity.relative_humidity);
//  delay(1000);
  disp.point(1);   // выкл точки
  disp.displayClockTwist(temp.temperature, humidity.relative_humidity, 25);    // выводим время
  delay(1000);
  disp.displayClockScroll(temp.temperature, humidity.relative_humidity, 25);    // выводим время
  delay(1000);


}
