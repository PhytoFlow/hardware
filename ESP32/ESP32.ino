#include <WiFi.h>
#include <WiFiUdp.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
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

// Variáveis
char incomingPacket[255]; // Buffer para dados recebidos
unsigned long tempoInicial;
const unsigned long tempoEspera = 5000;

// Cria um objeto JSON para armazenar as respostas
DynamicJsonDocument respostaJson(2048); // Espaço maior para o JSON

// AWS IoT Core configuração
const char* awsEndpoint = "awxktv65yt674-ats.iot.us-east-1.amazonaws.com";
const int awsPort = 8883;
const char* publishTopic = "irrigation/gateway/status";

// Certificados e chaves
const char* rootCA = R"EOF(
-----BEGIN CERTIFICATE-----

-----END CERTIFICATE-----
)EOF";

const char* certificatePemCrt = R"EOF(
-----BEGIN CERTIFICATE-----

-----END CERTIFICATE-----
)EOF";

const char* privatePemKey = R"EOF(
-----BEGIN RSA PRIVATE KEY-----

-----END RSA PRIVATE KEY-----
)EOF";

WiFiClientSecure secureClient;
PubSubClient mqttClient(secureClient);

String identificarNodemcusNaRede(const char* identifierRecebido) {
  Serial.println("Enviando comando IDENTIFIQUE-SE para os dispositivos na rede...");

  respostaJson["respostas"].clear();

  udp.beginPacket(broadcastIP, porta);
  udp.write((const uint8_t*)"IDENTIFIQUE-SE", strlen("IDENTIFIQUE-SE"));
  udp.endPacket();

  unsigned long tempoEsperaRespostas = millis() + 3000;
  String ipDoDispositivoEncontrado = "";

  while (millis() < tempoEsperaRespostas) {
    int packetSize = udp.parsePacket();
    if (packetSize) {
      int len = udp.read(incomingPacket, sizeof(incomingPacket));
      if (len > 0) {
        incomingPacket[len] = 0;
      }

      String resposta = String(incomingPacket);

      int idxIdentifier = resposta.indexOf("Identifier: ");
      int idxIP = resposta.indexOf("IP: ");
      
      if (idxIdentifier != -1 && idxIP != -1) {
        String identifier = resposta.substring(idxIdentifier + 12, idxIdentifier + 14);
        String ip = resposta.substring(idxIP + 4);

        if (identifier.length() == 2 && ip.length() > 0) {

          if (identifier == identifierRecebido) {
            return ip;
          }
        } else {
          Serial.println("Formato de resposta inválido para o Identifier ou IP.");
        }
      } else {
        Serial.println("Resposta não contém os campos 'Identifier' ou 'IP'.");
      }
    }
  }

  return "DISPOSITIVO_NAO_ENCONTRADO";
}

void messageReceived(char* topic, byte* payload, unsigned int length) {
  Serial.print("Mensagem recebida no tópico: ");
  Serial.println(topic);

  String payloadString;
  for (unsigned int i = 0; i < length; i++) {
    payloadString += (char)payload[i];
  }

  DynamicJsonDocument doc(1024);
  DeserializationError error = deserializeJson(doc, payloadString);

  if (error) {
    Serial.print("Erro ao parsear JSON: ");
    Serial.println(error.c_str());
    return;
  }

  if (doc.containsKey("identifier") && doc.containsKey("comand") && doc.containsKey("time")) {
    const char* identifier = doc["identifier"];
    const char* comand = doc["comand"];
    unsigned long time = doc["time"];

    if (strcmp(comand, "AGOAR") == 0) {

      String IPNodeMcu = identificarNodemcusNaRede(identifier);

      if (IPNodeMcu == "DISPOSITIVO_NAO_ENCONTRADO") {
        Serial.println("Dispositivo não encontrado com o Identificador: " + String(identifier));
      } else {
          DynamicJsonDocument doc(1024);
          doc["comand"] = comand;
          doc["time"] = time;

          String jsonString;
          serializeJson(doc, jsonString);

          udp.beginPacket(IPNodeMcu.c_str(), porta);
          udp.write((const uint8_t*)jsonString.c_str(), jsonString.length());
          udp.endPacket();

          Serial.println("Comando enviado para o NodeMCU: " + jsonString);
      }

    } else {
      Serial.println("Comando não reconhecido.");
    }

  } else {
    Serial.println("JSON recebido não contém os campos esperados.");
  }
}

void connectAWS() {
  secureClient.setCACert(rootCA);
  secureClient.setCertificate(certificatePemCrt);
  secureClient.setPrivateKey(privatePemKey);

  mqttClient.setServer(awsEndpoint, awsPort);
  mqttClient.setCallback(messageReceived);

  while (!mqttClient.connected()) {
    Serial.println("Conectando ao AWS IoT Core...");
    String clientID = "irrigation-gateway-000";
    if (mqttClient.connect(clientID.c_str())) {
      Serial.println("Conectado ao AWS IoT Core!");

      if (mqttClient.subscribe("irrigation/control/000/command")) {
        Serial.println("Inscrito no tópico: irrigation/control/000/command");
      } else {
        Serial.println("Falha ao inscrever no tópico.");
      }
    } else {
      Serial.print("Falha na conexão. Estado MQTT: ");
      Serial.println(mqttClient.state());
      delay(2000);
    }
  }
}


void setup() {
  Serial.begin(9600);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Conectando-se à rede Wi-Fi...");
  }
  Serial.println("Conectado à rede Wi-Fi!");

  udp.begin(porta);

  broadcastIP = WiFi.localIP();
  broadcastIP[3] = 255;

  respostaJson.createNestedArray("respostas");

  connectAWS();
}

unsigned long tempoUltimoEnvio = 0;
unsigned long intervaloEnvio = 120000;
bool coletaEmProgresso = false;
unsigned long tempoColetaInicio = 0;

void loop() {
  unsigned long tempoAtual = millis();

  if (!coletaEmProgresso && (tempoAtual - tempoUltimoEnvio >= intervaloEnvio)) {
    respostaJson["respostas"].clear();

    udp.beginPacket(broadcastIP, porta);
    udp.write((const uint8_t*)"DADOS", strlen("DADOS"));
    udp.endPacket();

    Serial.println("Comando DADOS enviado para todos os dispositivos.");

    tempoColetaInicio = tempoAtual;
    coletaEmProgresso = true;
  }

  if (coletaEmProgresso && (tempoAtual - tempoColetaInicio <= tempoEspera)) {
    int packetSize = udp.parsePacket();
    if (packetSize) {
      int len = udp.read(incomingPacket, sizeof(incomingPacket));
      if (len > 0) {
        incomingPacket[len] = 0;
      }

      String resposta = String(incomingPacket);
      respostaJson["respostas"].add(resposta);

      Serial.print("Resposta recebida: ");
      Serial.println(resposta);
    }
  }

  if (coletaEmProgresso && (tempoAtual - tempoColetaInicio > tempoEspera)) {
    Serial.println("Tempo de coleta de respostas encerrado.");

    JsonArray respostas = respostaJson["respostas"].as<JsonArray>();
    DynamicJsonDocument jsonFormatado(2048);
    JsonArray respostasFormatadas = jsonFormatado.to<JsonArray>();

    for (JsonVariant resposta : respostas) {
      String dado = resposta.as<String>();

      char identificador[16];
      float temperatura, umidade, umidade_solo, temperatura_solo;
      int intensidade_uv, intensidade_luz;

      if (sscanf(dado.c_str(), "Identificador: %[^,], Temperatura: %f, Umidade: %f, Umidade_Solo: %f, Intensidade_Luz: %d, Intensidade_UV: %d, Temperatura_Solo: %f",
                 identificador, &temperatura, &umidade, &umidade_solo, &intensidade_luz, &intensidade_uv, &temperatura_solo) == 7) {

        JsonObject item = respostasFormatadas.createNestedObject();
        item["identifier"] = identificador;
        JsonObject valores = item.createNestedObject("values");
        valores["temperature"] = temperatura;
        valores["humidity"] = umidade;
        valores["soil_humidity"] = umidade_solo;
        valores["light"] = intensidade_luz;
        valores["uv_intensity"] = intensidade_uv;
        valores["soil_temperature"] = temperatura_solo;
      }
    }

    if (mqttClient.state() != MQTT_CONNECTED) {
      Serial.println("Conexão MQTT perdida. Tentando reconectar...");
      connectAWS();
    }

    String jsonString;
    serializeJson(jsonFormatado, jsonString);
    Serial.println("Respostas formatadas em JSON:");
    Serial.println(jsonString);

    if (!mqttClient.publish(publishTopic, jsonString.c_str())) {
      Serial.print("Falha ao publicar no tópico MQTT. Estado MQTT: ");
      Serial.println(mqttClient.state());
      Serial.print("Tópico: ");
      Serial.println(publishTopic);
      Serial.print("Payload: ");
      Serial.println(jsonString);
    } else {
      Serial.println("Dados publicados no AWS IoT com sucesso.");
    }

    coletaEmProgresso = false;
    tempoUltimoEnvio = tempoAtual;
  }

  // Processa as mensagens MQTT
  mqttClient.loop();

  delay(10);
}
