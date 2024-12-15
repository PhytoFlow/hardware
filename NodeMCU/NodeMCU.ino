#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

const char* ssid = "Gabriel_Castro";
const char* password = "senha";

// Porta UDP para comunicação
const int porta = 8080;

// Inicializa o servidor UDP
WiFiUDP udp;

// Identidade do dispositivo
const String identity = "A1";  // Identidade do dispositivo NodeMCU

// Variáveis
char incomingPacket[255]; // Buffer para dados recebidos
String comandoRecebido;

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
    int len = udp.read(incomingPacket, sizeof(incomingPacket));
    if (len > 0) {
      incomingPacket[len] = 0; // Garante que a string seja terminada corretamente
    }
    comandoRecebido = String(incomingPacket);

    if (comandoRecebido == "DADOS") {

      String respostaArduino = SolicitarDadosArduino(comandoRecebido);
      EnviarDadosESP32(respostaArduino);

    } else {
      // Placeholder para outros comandos futuros
      Serial.print("Comando não reconhecido: ");
      Serial.println(comandoRecebido);
    }
  }

  delay(10);
}

String SolicitarDadosArduino(String comando){
  String respostaArduino = "";

  Serial.println("DADOS"); // Solicita dados ao arduino

  // Aguarda resposta do Arduino
  long startTime = millis();
  while (millis() - startTime < 3000) {
    if (Serial.available() > 0) {
      respostaArduino = Serial.readStringUntil('\n');
      break;
    }
  }

  return respostaArduino;
}

void EnviarDadosESP32(String respostaArduino){
  if (respostaArduino != "") {
    Serial.print("Resposta do Arduino: ");
    Serial.println(respostaArduino);

    // Adiciona o identificador (identity) à mensagem antes de enviar
    String messageToSend = "Identificador: " + identity + ", " + respostaArduino;

    // Envia a resposta de volta ao ESP32
    udp.beginPacket(udp.remoteIP(), udp.remotePort());
    udp.write(messageToSend.c_str());
    udp.endPacket();

  } else {
    Serial.println("Erro: Nenhuma resposta do Arduino.");
  }
}
