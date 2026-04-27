// SmartHome_Light.ino
// Controls the LED / relay on pin 13
// Receives '1' to turn ON, '0' to turn OFF via Serial at 9600 baud

const int LIGHT_PIN = 13;

void setup() {
  pinMode(LIGHT_PIN, OUTPUT);
  digitalWrite(LIGHT_PIN, LOW);
  Serial.begin(9600);
  
  // Brief flash to confirm startup
  digitalWrite(LIGHT_PIN, HIGH);
  delay(300);
  digitalWrite(LIGHT_PIN, LOW);
}

void loop() {
  if (Serial.available() > 0) {
    char cmd = Serial.read();
    
    if (cmd == '1') {
      digitalWrite(LIGHT_PIN, HIGH);  // Turn ON
      Serial.println("LIGHT_ON");
    } 
    else if (cmd == '0') {
      digitalWrite(LIGHT_PIN, LOW);   // Turn OFF
      Serial.println("LIGHT_OFF");
    }
  }
}