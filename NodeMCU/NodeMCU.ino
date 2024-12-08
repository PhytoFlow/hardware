#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

const char* ssid = "Gabriel_Castro";
const char* password = "qwertyui";

// Porta UDP para comunicação com o ESP32
const int porta = 8080;

// Inicializa o servidor UDP
WiFiUDP udp;

// Variáveis para armazenar os dados recebidos
String dataReceived = "";

const String identity = "NodeMCU";  // Identidade do dispositivo NodeMCU

void setup() {
  Serial.begin(9600);    // Porta serial do NodeMCU (com Arduino)
  Serial1.begin(9600);   // Porta serial para comunicação com o Arduino (pode ser outra taxa)

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Conectando-se à rede Wi-Fi...");
  }
  Serial.println("Conectado à rede Wi-Fi!");

  udp.begin(porta);

  // Envia pacote de broadcast para descobrir o ESP32
  IPAddress broadcastIP = WiFi.localIP();
  broadcastIP[3] = 255;  // Endereço de broadcast

  // Envia uma mensagem de "descoberta" para o ESP32
  udp.beginPacket(broadcastIP, porta);
  udp.write("DESCUBRA_ESP32");
  udp.endPacket();
}

void loop() {
  // Aguarda a resposta do ESP32 (IP)
  int packetSize = udp.parsePacket();

  String esp32IP = "";

  if (packetSize) {
    char incomingPacket[255];
    int len = udp.read(incomingPacket, sizeof(incomingPacket));
    if (len > 0) {
      incomingPacket[len] = 0;
    }

    // Recebe o IP do ESP32
    esp32IP = String(incomingPacket);
    Serial.print("IP do ESP32 recebido: ");
    Serial.println(esp32IP);
  }

  if(Serial.available() > 0){
    dataReceived = Serial.readStringUntil('\n');
    Serial.println("Dados recebidos do arduino: ");
    Serial.println(dataReceived);

    udp.beginPacket(esp32IP.c_str(), porta);
    udp.write(dataReceived.c_str());
    udp.endPacket();
  }

  delay(100);
}
