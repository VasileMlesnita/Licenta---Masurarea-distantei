void setup() {
  Serial.begin(9600);
}

void loop() {
  int potVal = analogRead(A0);  // 0...1023
  Serial.println(potVal);
  delay(20);  // suficient pentru 50Hz
}
