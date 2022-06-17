#define r 3
#define g 4
#define b 5

#define ENABLE_ADS

#ifdef ENABLE_ADS
#include<ADS1115_WE.h> 
#include<Wire.h>
#define ADS_I2C_ADDRESS       0x48
ADS1115_WE      adc = ADS1115_WE(ADS_I2C_ADDRESS);
#endif

const int numReadings = 10;
float readings[numReadings];    // the readings from the analog input
int readIndex = 0;              // the index of the current reading
float total = 0;                // the running total
float averageVoltage = 0;       // the average

void setup(){
    Wire.begin();
    Serial.begin(115200);
    pinMode(r,OUTPUT);
    pinMode(g,OUTPUT);
    pinMode(b,OUTPUT);
    Serial.println("ADS Averaging Test");
    Serial.println("15 June 2022 | Wednesday");
    Serial.println("Initializing ADS1115 in 1 seconds...");
    delay(1000);

    #ifdef ENABLE_ADS
    if(!adc.init()){
        Serial.println("ADS1115 not connected!");
    }
    adc.setVoltageRange_mV(ADS1115_RANGE_6144); //comment line/change parameter to change range
    #endif

    Serial.println("ADS1115 Example Sketch - Single Shot Mode");
    Serial.println("Channel / Voltage [V]: ");
    Serial.println();

    for (int thisReading = 0; thisReading < numReadings; thisReading++) {
        readings[thisReading] = 0;
    }

}

void loop(){

  #ifdef ENABLE_ADS
  float ch0_raw = readChannel(ADS1115_COMP_0_GND);    
  float ch0 = readChannelAverage(ADS1115_COMP_0_GND);    
  Serial.printf("ch0 raw: %f ch0 average: %f \n", ch0_raw, ch0);
  delay(100);

  #else
  float ch0 = -1;
  #endif

  
}

#ifdef ENABLE_ADS

void read_ads1115(){
  float ch0 = readChannel(ADS1115_COMP_0_GND);    
  float ch1 = readChannel(ADS1115_COMP_1_GND);    
  float ch2 = readChannel(ADS1115_COMP_2_GND);    
  float ch3 = readChannel(ADS1115_COMP_3_GND);  
  String ads_readout = "ch0: "+ String(ch0)+"V\t ch1: "+ String(ch1)+"V\t ch2: "+ String(ch2)+"V\t ch3: "+ String(ch3)+"V";
}

float readChannel(ADS1115_MUX channel) {
  float voltage = 0.0;
  adc.setCompareChannels(channel);
  adc.startSingleMeasurement();
  while(adc.isBusy()){}
  voltage = adc.getResult_V(); // alternative: getResult_mV for Millivolt
  return voltage;
}

float readChannelAverage(ADS1115_MUX channel) {
  float voltage = 0.0;
  while(adc.isBusy()){}
  while(readIndex < numReadings){
    adc.setCompareChannels(channel);  // keep on reading the channel
    adc.startSingleMeasurement();     // keep on measuring
    voltage = adc.getResult_V();      // alternative: getResult_mV for Millivolt
    total = total - readings[readIndex];
    readings[readIndex] = voltage;
    total = total + readings[readIndex];
    readIndex = readIndex + 1;
    averageVoltage = total / numReadings;
    if (readIndex >= numReadings) {
      readIndex = 0;
      return averageVoltage;
    }
    delay(100);        // delay in between reads for ADS stability
  }
  return voltage;
}

#endif

void red_on(){
  digitalWrite(r,1);
  digitalWrite(g,0);
  digitalWrite(b,0);
}
void green_on(){
  digitalWrite(r,0);
  digitalWrite(g,1);
  digitalWrite(b,0);
}
void blue_on(){
  digitalWrite(r,0);
  digitalWrite(g,0);
  digitalWrite(b,1);
}
void rgb_on(){
  digitalWrite(r,1);
  digitalWrite(g,1);
  digitalWrite(b,1);
}
void rgb_off(){
  digitalWrite(r,0);
  digitalWrite(g,0);
  digitalWrite(b,0);
}
