#include "GroveBase-ESPDuino32-Mapping.h"

const int trig_pin = GROVE_D6; // D7 default
const int echo_pin = GROVE_D7; // D6 default

// Sound speed in air
#define SOUND_SPEED 340
#define TRIG_PULSE_DURATION_US 10


long ultrason_duration;
float distance_cm;

void setup() {
  Serial.begin(115200);
  pinMode(trig_pin, OUTPUT); // We configure the trig as output
  pinMode(echo_pin, INPUT); // We configure the echo as input
}

void loop() {
  // Set up the signal
  digitalWrite(trig_pin, LOW);
  delayMicroseconds(2);
 // Create a 10 µs impulse
  digitalWrite(trig_pin, HIGH);
  delayMicroseconds(TRIG_PULSE_DURATION_US);
  digitalWrite(trig_pin, LOW);

  // Return the wave propagation time (in µs)
  ultrason_duration = pulseIn(echo_pin, HIGH);

//distance calculation
  distance_cm = ultrason_duration * SOUND_SPEED/2 * 0.0001;

  // We print the distance on the serial port
  Serial.print("Distance (cm): ");
  Serial.println(distance_cm);

  delay(100);
}
 