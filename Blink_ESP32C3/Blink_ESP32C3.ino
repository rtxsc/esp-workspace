#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_BMP280.h>
#define LDR 1
#define R 3 //red
#define G 4 //red
#define B 5 //red
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET     4 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
Adafruit_BMP280 bmp; // use I2C interface
Adafruit_Sensor *bmp_temp = bmp.getTemperatureSensor();
Adafruit_Sensor *bmp_pressure = bmp.getPressureSensor();

// the setup function runs once when you press reset or power the board
void setup() {
  // initialize digital pin LED_BUILTIN as an output.
  pinMode(R, OUTPUT);
  pinMode(G, OUTPUT);
  pinMode(B, OUTPUT);
  Serial.begin(115200);
  Serial.println("ESP32-C3");
 if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    digitalWrite(R, HIGH); 
    digitalWrite(G, LOW); 
    digitalWrite(B, LOW); 
    for(;;); // Don't proceed, loop forever
  }
  display.setTextSize(2);             // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE);        // Draw white text
  display.setCursor(0,0);             // Start at top-left corner
  display.println(F("Hello, ESP32-C3!"));

  delay(1000);
  display.clearDisplay();

  unsigned status;
  //status = bmp.begin(BMP280_ADDRESS_ALT, BMP280_CHIPID);
  status = bmp.begin(0x76);
  if (!status) {
    Serial.println(F("Could not find a valid BMP280 sensor, check wiring or "
                      "try a different address!"));
    Serial.print("SensorID was: 0x"); Serial.println(bmp.sensorID(),16);
    Serial.print("        ID of 0xFF probably means a bad address, a BMP 180 or BMP 085\n");
    Serial.print("   ID of 0x56-0x58 represents a BMP 280,\n");
    Serial.print("        ID of 0x60 represents a BME 280.\n");
    Serial.print("        ID of 0x61 represents a BME 680.\n");
    while (1) delay(10);
  }

  /* Default settings from datasheet. */
  bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,     /* Operating Mode. */
                  Adafruit_BMP280::SAMPLING_X2,     /* Temp. oversampling */
                  Adafruit_BMP280::SAMPLING_X16,    /* Pressure oversampling */
                  Adafruit_BMP280::FILTER_X16,      /* Filtering. */
                  Adafruit_BMP280::STANDBY_MS_500); /* Standby time. */

  bmp_temp->printSensorDetails();
}

// the loop function runs over and over again forever
void loop() {
  int read_ldr = analogRead(LDR);

  sensors_event_t temp_event, pressure_event;
  bmp_temp->getEvent(&temp_event);
  bmp_pressure->getEvent(&pressure_event);

  display.setCursor(0,16);             // Start at top-left corner
  display.setTextSize(1);
  display.println(F("Temp"));
  display.setTextSize(2); 
  display.println(int(temp_event.temperature), DEC);
  display.setTextSize(1);
  display.println(F("Pressure / Light"));
  display.setTextSize(2);
  display.print(int(pressure_event.pressure), DEC);
//  display.print(F(" "));
//  display.println(int(read_ldr), DEC);

  
//  display.setTextSize(1);             // Normal 1:1 pixel scale
//  display.setTextColor(SSD1306_WHITE);        // Draw white text
  display.setCursor(0,0);             // Start at top-left corner
//  display.println(F("Hello, world!"));
//  
  display.setTextSize(2); 
  display.setTextColor(SSD1306_BLACK, SSD1306_WHITE); // Draw 'inverse' text
  display.println(F("RED"));

//  display.setTextSize(2);             // Draw 2X-scale text
//  display.setTextColor(SSD1306_WHITE);
//  display.print(F("0x")); display.println(0xDEADBEEF, HEX);

  display.display();
  Serial.println("RED");
  digitalWrite(R, HIGH); 
  digitalWrite(G, LOW); 
  digitalWrite(B, LOW);
  delay(100);       
  display.setCursor(0,0);             // Start at top-left corner
  display.setTextSize(2); 
  display.setTextColor(SSD1306_BLACK, SSD1306_WHITE); // Draw 'inverse' text
  display.println(F("GREEN"));
  display.display();

  Serial.println("GREEN");
  digitalWrite(R, LOW); 
  digitalWrite(G, HIGH); 
  digitalWrite(B, LOW); 
  delay(100);     
  display.setCursor(0,0);             // Start at top-left corner
  display.setTextSize(2); 
  display.setTextColor(SSD1306_BLACK, SSD1306_WHITE); // Draw 'inverse' text
  display.println(F("BLUE"));
  display.display();
  Serial.println("BLUe");
  digitalWrite(R, LOW); 
  digitalWrite(G, LOW); 
  digitalWrite(B, HIGH); 
  delay(100);      
  display.clearDisplay();
               
}
