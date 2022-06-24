void setup() {
  // initialize serial communications (for debugging only):
  Serial.begin(115200);
}

void loop() {

  // beep_once();
  // delay(1000);
  // beep_twice();
  // delay(1000);
  // beep_thrice();
  // delay(1000);
  siren();
  delay(1000);

  
}

void beep_once(){
  tone(8, 1500, 150);
}

void beep_twice(){
  tone(8, 1500, 50);
  delay(100);
  tone(8, 1500, 50);
  delay(100);
}

void beep_thrice(){
  tone(8, 1500, 50);
  delay(100);
  tone(8, 1500, 50);
  delay(100);
  tone(8, 1500, 50);
  delay(100);
}

void siren(){
  int min_tone = 700;
  int max_tone = 1500;
  for(int i=min_tone ; i <= max_tone ; i++){
      tone(8, i, 20);
      delay(1);        // delay in between reads for stability
  }
  for(int i=max_tone ; i > min_tone ; i--){
      tone(8, i, 20);
      delay(1);        // delay in between reads for stability
  }
}
