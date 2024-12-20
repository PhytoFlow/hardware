#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <ArduinoJson.h>

const char* ssid = "Gabriel_Castro";
const char* password = "senha";

const int porta = 8080;

WiFiUDP udp;

const String identity = "A1";

char incomingPacket[255];
String comandoRecebido;

const int pinoAguar = D8;

void setup() {
  Serial.begin(9600);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Conectando-se à rede Wi-Fi...");
  }
  Serial.println("Conectado à rede Wi-Fi!");

  udp.begin(porta);

  pinMode(pinoAguar, OUTPUT);
  digitalWrite(pinoAguar, LOW);
}

void loop() {
  int packetSize = udp.parsePacket();
  if (packetSize) {
    int len = udp.read(incomingPacket, sizeof(incomingPacket));
    if (len > 0) {
      incomingPacket[len] = 0;
    }
    comandoRecebido = String(incomingPacket);

    if (comandoRecebido == "DADOS") {

      String respostaArduino = SolicitarDadosArduino(comandoRecebido);
      EnviarDadosESP32(respostaArduino);

    } else if(comandoRecebido == "IDENTIFIQUE-SE") {
        String respostaIdentificacao = String("Identifier: ") + identity +
                                              ", IP: " + WiFi.localIP().toString();
        udp.beginPacket(udp.remoteIP(), udp.remotePort());
        udp.write(respostaIdentificacao.c_str());
        udp.endPacket();
        Serial.println("Resposta de IDENTIFIQUE-SE enviada.");
    } else {
      DynamicJsonDocument doc(1024);
      DeserializationError error = deserializeJson(doc, comandoRecebido);

      if (!error) {
        const char* comand = doc["comand"];
        if (comand && String(comand) == "AGUAR") {
          int tempo = doc["time"];
          if (tempo > 0) {
            AguarSetor(tempo);
          }
        }
      } else {
        Serial.println("Erro ao processar JSON.");
      }
    }
  }

  delay(10);
}

String SolicitarDadosArduino(String comando){
  String respostaArduino = "";

  Serial.println("DADOS");

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

    String messageToSend = "Identificador: " + identity + ", " + respostaArduino;

    udp.beginPacket(udp.remoteIP(), udp.remotePort());
    udp.write(messageToSend.c_str());
    udp.endPacket();

  } else {
    Serial.println("Erro: Nenhuma resposta do Arduino.");
  }
}

void AguarSetor(int tempo) {
  digitalWrite(pinoAguar, HIGH);
  Serial.println("Aguar ativado!");

  delay(tempo);

  digitalWrite(pinoAguar, LOW);
  Serial.println("Aguar desativado.");
}
