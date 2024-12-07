#include <WiFi.h>
#include <WiFiUdp.h>

// Defina os dados da sua rede Wi-Fi
const char* ssid = "Gabriel_Castro";
const char* password = "senha";

// Porta UDP para comunicação
const int porta = 8080;

// Inicializa o servidor UDP
WiFiUDP udp;

// Variáveis para armazenar os dados recebidos
char incomingPacket[255];

void setup() {
  Serial.begin(9600);

  // Conectando-se à rede Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Conectando-se à rede Wi-Fi...");
  }
  Serial.println("Conectado à rede Wi-Fi!");

  // Inicia o servidor UDP
  udp.begin(porta);
}

void loop() {
  // Verifica se há pacotes UDP recebidos
  int packetSize = udp.parsePacket();
  if (packetSize) {
    // Lê os dados recebidos
    int len = udp.read(incomingPacket, sizeof(incomingPacket));
    if (len > 0) {
      incomingPacket[len] = 0;  // Garantir que a string seja terminada corretamente
    }

    // Exibe os dados recebidos no monitor serial
    Serial.print("Dados recebidos do NodeMCU: ");
    Serial.println(incomingPacket);
  }

  delay(100); // Para evitar sobrecarga
}
