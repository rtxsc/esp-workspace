#include <Adafruit_AHT10.h>
#include <Wire.h>

#define RED     3
#define GRN     4
#define BLU     5
#define I2C_SDA 8 // 0 for ESP01s
#define I2C_SCL 9 // 2 for ESP01s

#define CLK 18
#define DIO 19

Adafruit_AHT10 aht;

#include "GyverTM1637.h"
GyverTM1637 disp(CLK, DIO);

// setting PWM properties
const int freq = 5000;
const int ledChannelR = 0;
const int ledChannelG = 1;
const int ledChannelB = 2;

const int resolution = 12; // Resolution 8, 10, 12, 15
const int max_adc = (pow(2,resolution)-1)/3;

void setup() {
  Serial.begin(115200);
  Wire.begin(I2C_SDA, I2C_SCL);
  disp.clear();
  disp.brightness(4);
  pinMode(RED, OUTPUT);
  pinMode(GRN, OUTPUT);
  pinMode(BLU, OUTPUT);

    // configure LED PWM 
  ledcSetup(ledChannelR, freq, resolution);
  ledcSetup(ledChannelG, freq, resolution);
  ledcSetup(ledChannelB, freq, resolution);

  // attach the channel to the GPIO to be controlled
  ledcAttachPin(RED, ledChannelR);
  ledcAttachPin(GRN, ledChannelG);
  ledcAttachPin(BLU, ledChannelB);

  Serial.println("Adafruit AHT10 demo!");

  if (! aht.begin()) {
    Serial.println("Could not find AHT10? Check wiring");
    while (1) delay(10);
  }
  Serial.println("AHT10 found");
  digitalWrite(RED,1);
  digitalWrite(GRN,0);
  digitalWrite(BLU,0);
  delay(500);
  digitalWrite(RED,0);
  digitalWrite(GRN,1);
  digitalWrite(BLU,0);
  delay(500);
  digitalWrite(RED,0);
  digitalWrite(GRN,0);
  digitalWrite(BLU,1);
  delay(500);
  digitalWrite(RED,0);
  digitalWrite(GRN,0);
  digitalWrite(BLU,0);
  delay(500);

}

void loop() {
  sensors_event_t humidity, temp;
  aht.getEvent(&humidity, &temp);// populate temp and humidity objects with fresh data
  float tempc = temp.temperature;
  float humid = humidity.relative_humidity;

  // // DUMMY SECTION STARTS
  // float dummy_tempc = analogRead(0);
  // dummy_tempc = map(dummy_tempc,0,4095,15,60);
  // tempc = dummy_tempc;
  // // DUMMY SECTION ENDS

  // Serial.print("Temperature: "); Serial.print(tempc); Serial.println(" degrees C");
  // Serial.print("Humidity: "); Serial.print(humid); Serial.println("% rH");

  map_rgb(tempc);

  disp.point(1);   // выкл точки
  disp.displayClockTwist(tempc,humid,20);    // выводим время
  delay(100);

}

void map_rgb(float temp){
    float grn_val = 0.0;
    float blu_val = 0.0;
    float red_val = 0.0;

    if(temp < 30){
      // TOTALLY BLUE AREA
      ledcWrite(ledChannelR,0);
      ledcWrite(ledChannelG,0);
      ledcWrite(ledChannelB,max_adc);
    }
    else if(temp >= 30 && temp < 50){
      // BLUE TO GREEN AREA
      blu_val = map(temp, 20, 34.9, max_adc, 0);  // blue --
      blu_val = constrain(blu_val, 0,max_adc);
      grn_val = max_adc-blu_val;                  // green ++
      ledcWrite(ledChannelR,0);
      ledcWrite(ledChannelG,grn_val);
      ledcWrite(ledChannelB,blu_val);
    }
    else if(temp == 50){
      // TOTALLY GREEN AREA
      ledcWrite(ledChannelR,0);
      ledcWrite(ledChannelG,max_adc);
      ledcWrite(ledChannelB,0);
    }
    else if(temp > 50  && temp < 55){ 
      // GREEN TO RED AREA
      red_val = map(temp, 35, 49.9, 0, max_adc); // red ++
      red_val = constrain(red_val, 0, max_adc);
      grn_val = max_adc-red_val;                 // green --
      ledcWrite(ledChannelR,red_val);
      ledcWrite(ledChannelG,grn_val);
      ledcWrite(ledChannelB,0);
    }
    else{
      // exceeding 55 degree celcius
      // TOTALLY RED AREA
      ledcWrite(ledChannelR,max_adc);
      ledcWrite(ledChannelG,0);
      ledcWrite(ledChannelB,0);
    }


}