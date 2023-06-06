#include "GyverTM1637.h" // for BIG VERSION
#include "HX711.h"
#include <EEPROM.h>
#define SCALE_PARAM_ADDRESS 0x05
#define LOADCELL_200KG
// #define LOADCELL_20KG

#ifdef LOADCELL_200KG
// #define ADC_NO_LOAD_200KG 62000 // min 63000 max
// #define ADC_CAL_MIN_200KG 29000
// #define ADC_CAL_MAX_200KG 30000

#define ADC_NO_LOAD_200KG 47900 // min 63000 max
#define ADC_CAL_MIN_200KG 19000
#define ADC_CAL_MAX_200KG 30000

#else
#define ADC_NO_LOAD_20KG  83000 // min 84000 max
#define ADC_CAL_MIN_20KG  153000
#define ADC_CAL_MAX_20KG  153500
#endif

#define CLK           3  // GYVER
#define DIO           4  // GYVER
GyverTM1637               disp(CLK, DIO);

// HX711 circuit wiring
const int LOADCELL_SCK_PIN = 5;
const int LOADCELL_DOUT_PIN = 6;
double KNOWN_WEIGHT = 1.46; // 1.46kg = 1.5l bottle
const byte button = 2;
double scale_param = 0;
byte calibrate_count = 0;
HX711 scale;

int readIntFromEEPROM(int address)
{
  return (EEPROM.read(address) << 8) + EEPROM.read(address + 1);
}

void writeIntIntoEEPROM(int address, int number)
{ 
  EEPROM.write(address, number >> 8);
  EEPROM.write(address + 1, number & 0xFF);
}
  
void setup() {
  pinMode(button, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(button), press_to_calibrate, FALLING);

  disp.clear();
  disp.brightness(7);  // яркость, 0 - 7 (минимум - максимум)
  byte test[4] = {_t, _e, _S, _t};
  disp.point(0);   
  disp.twistByte(test, 25);
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial.println("HX711 Calibration Loop");
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  for(int i=0; i<4; i++){
      Serial.print("Make sure no object is on top of the scale - ");
      Serial.println(4-i);
      delay(100);
  }
  scale_param = readIntFromEEPROM(SCALE_PARAM_ADDRESS);
  Serial.print("[EEPROM] Reading previous scale parameter from 0x05: ");
  Serial.println(scale_param);
  delay(1000);
  scale.set_scale(scale_param);
  scale.tare(); 

}
byte read_count = 2;
void loop() {

  if(calibrate_count > 0) calibrate();
  // put your main code here, to run repeatedly:
  Serial.print("one reading:\t");
  Serial.print(scale.get_units(), 2);
  Serial.print("\t| average:\t ");
  Serial.print(scale.get_units(read_count), 2);
  Serial.print(" read average:\t");
  Serial.print(scale.read_average(2));  	// print the average of 20 readings from the ADC

  float weight = scale.get_units(read_count);
  if(weight < 00.00) weight = 00.00;
  if(weight > 99.99) weight = 99.99;

  Serial.print("\t| weight(kg):\t");
  Serial.println(weight);
   byte weight_array[4];
    int weight_array_int = weight*100; 

    for (int i = 3 ; i >= 0 ; i--)
    {
      weight_array[i] = weight_array_int % 10;
      weight_array_int /= 10 ;
    }
    disp.clear();
    disp.point(1);   
    disp.scroll(weight_array, 20);   

  scale.power_down();              // put the ADC in sleep mode
  delay(500);
  scale.power_up();

}

void calibrate(){
  Serial.println();
  Serial.println();
  Serial.print("[CALIBRATION] Re-calibration requested");
  byte cali[4] = {_C, _a, _L, _i};
  disp.point(0);   
  disp.twistByte(cali, 25);
  delay(100);

  byte clear_item[] = {_c, _l, _e, _a, _r, _empty, _o, _b, _j, _empty,};
  disp.runningString(clear_item, sizeof(clear_item), 200);  

  Serial.println("Step 1 : Set scale with no parameter. DO NOT PLACE THE LOAD");
  scale.set_scale();
  Serial.println("Step 2 : Set tare with no parameter. DO NOT PLACE THE LOAD");
  scale.tare();

  Serial.println("Before setting up the scale:");
  Serial.print("read 24-bit ADC: \t\t");
  Serial.println(scale.read());			// print a raw reading from the ADC

  Serial.print("read average: \t\t");
  Serial.println(scale.read_average(20));  	// print the average of 20 readings from the ADC

  Serial.print("get offset: \t\t");
  Serial.println(scale.get_offset());		// print the average of 5 readings from the ADC minus the tare weight (not set yet)

  Serial.print("get value: \t\t");
  Serial.println(scale.get_value(5),2);		// print the average of 5 readings from the ADC minus the tare weight (not set yet)

  Serial.print("get units: \t\t");
  Serial.println(scale.get_units(5), 2);	// print the average of 5 readings from the ADC minus tare weight (not set) divided
						// by the SCALE parameter (not set yet)

  byte place_item[] = {_P, _l, _a, _c, _e, _empty, _o, _b, _j, _empty,};
  disp.runningString(place_item, sizeof(place_item), 200);  
  Serial.println("Step 3 : Place a known weight on the sensor in");
  for(byte i=0; i<4; i++){
      Serial.print("countdown T-minus ");
      Serial.println(4-i);
      disp.clear();
      byte c = 4 - i;
     disp.displayInt(c);      
     delay(500);
  }
  Serial.println("Step 4 : Measuring weight");
  double w = scale.get_units(10);  // must achieve 153000 < w < 153500 for 1.46 kg
  Serial.print("weight measured (w):");
  Serial.println(w); 
  delay(100);

  #ifdef LOADCELL_200KG
  uint32_t min_expected = ADC_CAL_MIN_200KG;
  uint32_t max_expected = ADC_CAL_MAX_200KG;
  #else
  uint32_t min_expected = ADC_CAL_MIN_20KG;
  uint32_t max_expected = ADC_CAL_MAX_20KG;
  #endif

  if(w < min_expected || w > max_expected){
    do{
      w = scale.get_units(10); // must achieve 153000 < w < 153500 for 1.46 kg
      Serial.print("Target ");
      Serial.print(String(min_expected) + " < w < " + String(max_expected));
      Serial.print(" not achieved [RETRY] getting (w):");
      Serial.println(w);
      byte fail[4] = {_F, _a, _i, _L};
      disp.clear();
      disp.point(0);   
      disp.twistByte(fail, 25); 
      delay(100);
    }while(w < min_expected || w > max_expected);
  }
  // get the ratio of ADC per unit known weight (ADC)
  scale_param = w / KNOWN_WEIGHT; //  must achieve 103810.96
  Serial.println("[EEPROM] Saving new scale parameter into 0x05");
  writeIntIntoEEPROM(SCALE_PARAM_ADDRESS, scale_param); // this MUST be saved in case of power failure!
  delay(100);
  Serial.print("scale parameter obtained (w/KNOWN_WEIGHT):");
  Serial.println(scale_param);
  delay(100);
  Serial.print("Step 5 : setting up scale parameter with value of: ");
  Serial.println(scale_param);
  scale.set_scale(scale_param);
  delay(100);
  byte done[4] = {_d, _o, _n, _e};
  disp.point(0);   
  disp.twistByte(done, 25);
  delay(100);
  
  byte remove_item[] = {_G, _e, _t, _empty, _o, _b, _j, _empty,};
  disp.runningString(remove_item, sizeof(remove_item), 200);  
  Serial.println("Step 6 : Reset scale to 0 with tare(). PLEASE REMOVE THE LOAD NOW!!!");
    for(byte i=0; i<4; i++){
      Serial.print("PLEASE REMOVE THE LOAD NOW!!! countdown T-minus ");
      Serial.println(4-i);
      disp.clear();
      byte c = 4 - i;
      disp.displayInt(c);
      delay(500);
  }
  scale.tare(); // getting the offset again

  Serial.println("After setting up the scale:");

  Serial.print("Step 7 :read 24-bit ADC:: \t\t");
  Serial.println(scale.read());                 // print a raw reading from the ADC

  Serial.print("Step 8 :read average: \t\t");
  Serial.println(scale.read_average(20)); 

  Serial.print("get offset after calibration: \t\t");
  Serial.println(scale.get_offset());		// print the average of 5 readings from the ADC minus the tare weight (not set yet)

  Serial.print("Step 9 : get value after calibration: \t\t");
  Serial.println(scale.get_value(5),2);    // print the average of 5 readings from the ADC minus the tare weight, set with tare()
  delay(100);

  Serial.print("Step 10 : get units after calibration: \t\t");
  Serial.println(scale.get_units(5), 2);        // print the average of 5 readings from the ADC minus tare weight, divided
            // by the SCALE parameter set with set_scale
  delay(100);
  disp.twistByte(done, 25);
  delay(100);
  calibrate_count = 0;

}

void press_to_calibrate(){
  calibrate_count++;
}
