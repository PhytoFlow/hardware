#include <WiFi.h>
#include <WiFiUdp.h>
#include <ArduinoJson.h>

// Defina os dados da sua rede Wi-Fi
const char* ssid = "Gabriel_Castro";
const char* password = "senha";

// Porta UDP para comunicação
const int porta = 8080;

// Inicializa o servidor UDP
WiFiUDP udp;

// Endereço de Broadcast
IPAddress broadcastIP;

// Botão para iniciar a comunicação 
const int botaoPin = 4;

// Variáveis
char incomingPacket[255]; // Buffer para dados recebidos
bool botaoPressionado = false;
unsigned long tempoInicial;
const unsigned long tempoEspera = 5000;

// Cria um objeto JSON para armazenar as respostas
DynamicJsonDocument respostaJson(2048); // Espaço maior para o JSON

void setup() {
  Serial.begin(9600);

  pinMode(botaoPin, INPUT_PULLUP);

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
  broadcastIP[3] = 255; // Broadcast na última parte do IP da rede

  // Cria o array de respostas no JSON
  respostaJson.createNestedArray("respostas");
}

void loop() {
  // Verifica se o botão foi pressionado
  if (digitalRead(botaoPin) == LOW && !botaoPressionado) {
    botaoPressionado = true;
    tempoInicial = millis();

    // Limpa as respostas anteriores no JSON
    respostaJson["respostas"].clear();

    Serial.print("Botão apertado");

    // Envia comando de solicitação de dados para todos os dispositivos
    udp.beginPacket(broadcastIP, porta);
    udp.write((const uint8_t*)"DADOS", strlen("DADOS"));
    udp.endPacket();

    Serial.println("Comando DADOS enviado para todos os dispositivos.");
  }

  // Aguarda e processa respostas
  if (botaoPressionado && millis() - tempoInicial <= tempoEspera) {
    int packetSize = udp.parsePacket();
    if (packetSize) {
      int len = udp.read(incomingPacket, sizeof(incomingPacket));
      if (len > 0) {
        incomingPacket[len] = 0; // Garante que a string seja terminada corretamente
      }

      // Serializa a resposta recebida no JSON
      String resposta = String(incomingPacket); // Converte para String

      // Adiciona a resposta como um item no array de "respostas"
      respostaJson["respostas"].add(resposta);

      Serial.print("Resposta recebida: ");
      Serial.println(resposta);
    }
  } else if (botaoPressionado && millis() - tempoInicial > tempoEspera) {
    botaoPressionado = false;
    Serial.println("Tempo de coleta de respostas encerrado.");

    // Processa e formata as respostas recebidas
    JsonArray respostas = respostaJson["respostas"].as<JsonArray>();
    DynamicJsonDocument jsonFormatado(2048);
    JsonArray respostasFormatadas = jsonFormatado.createNestedArray("respostas");

    for (JsonVariant resposta : respostas) {
      String dado = resposta.as<String>();

      // Variáveis temporárias para armazenar os valores extraídos
      char identificador[16];
      float temperatura, umidade, umidade_solo, temperatura_solo;
      int intensidade_uv;

      // Extração dos valores com sscanf
      if (sscanf(dado.c_str(), "Identificador: %[^,], Temperatura: %f, Umidade: %f, Umidade Solo: %f, Intensidade UV: %d, Temperatura Solo: %f",
                identificador, &temperatura, &umidade, &umidade_solo, &intensidade_uv, &temperatura_solo) == 6) {
        // Cria um objeto JSON para cada resposta formatada
        JsonObject item = respostasFormatadas.createNestedObject();
        item["identificador"] = identificador;
        JsonObject valores = item.createNestedObject("valores");
        valores["temperatura"] = temperatura;
        valores["umidade"] = umidade;
        valores["umidade_solo"] = umidade_solo;
        valores["intensidade_uv"] = intensidade_uv;
        valores["temperatura_solo"] = temperatura_solo;
      }
    }

    // Converte o JSON para string para visualização ou envio
    String jsonString;
    serializeJson(jsonFormatado, jsonString);
    Serial.println("Respostas formatadas em JSON:");
    Serial.println(jsonString);

    // Aqui você pode enviar o JSON para a AWS via MQTT ou HTTP, por exemplo.
  }

  delay(10);
}
