void setup() {
  Serial.begin(9600);
}

void loop() {
  if (Serial.available() > 0) {
    String dataReceived = Serial.readStringUntil('\n');
    Serial.println("Dados recebidos do Arduino: ");
    Serial.println(dataReceived);
  }
}
