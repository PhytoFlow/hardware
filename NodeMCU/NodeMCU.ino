#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

const char* ssid = "Gabriel_Castro";
const char* password = "senha";

const char* esp32_ip = "192.168.217.253";
const int esp32_porta = 8080;

WiFiUDP udp;

String dataReceived = "";

void setup() {
  Serial.begin(9600);
  
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Conectando-se à rede Wi-Fi...");
  }
  Serial.println("Conectado à rede Wi-Fi!");

  udp.begin(esp32_porta);
}

void loop() {
  if (Serial.available() > 0) {
    // Recebe os dados enviados pelo Arduino via Serial
    dataReceived = Serial.readStringUntil('\n');
    Serial.print("Dados recebidos do Arduino: ");
    Serial.println(dataReceived);

    // Envia os dados para o ESP32 via UDP
    udp.beginPacket(esp32_ip, esp32_porta);
    udp.write(dataReceived.c_str());
    udp.endPacket();
  }

  delay(100); // Para evitar sobrecarga
}
