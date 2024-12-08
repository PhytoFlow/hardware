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

// Endereço de Broadcast
IPAddress broadcastIP;

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

  // Calcula o endereço de broadcast
  broadcastIP = WiFi.localIP();
  broadcastIP[3] = 255;  // Broadcast na última parte do IP da rede
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
    Serial.print("Dados recebidos: ");
    Serial.println(incomingPacket);

    // Se o pacote recebido for uma solicitação de "descoberta"
    if (String(incomingPacket) == "DESCUBRA_ESP32") {
      // Responde com o IP do ESP32
      String esp32IP = WiFi.localIP().toString(); // IP do ESP32 como string
      udp.beginPacket(udp.remoteIP(), udp.remotePort());

      // Envia o IP como bytes (não é necessário converter explicitamente para uint8_t*)
      udp.write(reinterpret_cast<const uint8_t*>(esp32IP.c_str()), esp32IP.length()); // Passa a string como uint8_t*
      udp.endPacket();
    }
  }

  delay(100); // Para evitar sobrecarga
}
