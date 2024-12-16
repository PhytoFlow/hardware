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

// Botão para iniciar a comunicação 
const int botaoPinSolicitarDados = 4;

// Variáveis
char incomingPacket[255]; // Buffer para dados recebidos
bool botaoPressionado = false;
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

void connectAWS() {
  secureClient.setCACert(rootCA);
  secureClient.setCertificate(certificatePemCrt);
  secureClient.setPrivateKey(privatePemKey);

  mqttClient.setServer(awsEndpoint, awsPort);
  mqttClient.setKeepAlive(240); // Aumenta o keep-alive para evitar desconexão

  // Tentando a conexão
  while (!mqttClient.connected()) {
    Serial.println("Conectando ao AWS IoT Core...");
    // Usando um clientID único
    String clientID = "irrigation-gateway-000";  // Ou o clientID que você configurou
    if (mqttClient.connect(clientID.c_str())) {
      Serial.println("Conectado ao AWS IoT Core!");
    } else {
      Serial.print("Falha na conexão. Estado MQTT: ");
      Serial.println(mqttClient.state());
      delay(2000); // Dê mais tempo para a reconexão
    }
  }
}


void setup() {
  Serial.begin(9600);

  pinMode(botaoPinSolicitarDados, INPUT_PULLUP);

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

  // Conecta ao AWS IoT Core
  connectAWS();
}

void loop() {
  // Verifica se o botão foi pressionado
  if (digitalRead(botaoPinSolicitarDados) == LOW && !botaoPressionado) {
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
    JsonArray respostasFormatadas = jsonFormatado.to<JsonArray>(); // Ajuste aqui: cria um array diretamente no JSON raiz

    for (JsonVariant resposta : respostas) {
        String dado = resposta.as<String>();

        // Variáveis temporárias para armazenar os valores extraídos
        char identificador[16];
        float temperatura, umidade, umidade_solo, temperatura_solo;
        int intensidade_uv, intensidade_luz;

        // Extração dos valores com sscanf
        if (sscanf(dado.c_str(), "Identificador: %[^,], Temperatura: %f, Umidade: %f, Umidade_Solo: %f, Intensidade_Luz: %d, Intensidade_UV: %d, Temperatura_Solo: %f",
                   identificador, &temperatura, &umidade, &umidade_solo, &intensidade_luz, &intensidade_uv, &temperatura_solo) == 7) {
            // Cria um objeto JSON para cada resposta formatada
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
        connectAWS();  // Reconectar
    }

    // Converte o JSON para string para envio
    String jsonString;
    serializeJson(jsonFormatado, jsonString);
    Serial.println("Respostas formatadas em JSON:");
    Serial.println(jsonString);

    // Publica o JSON no tópico MQTT
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
  }

  // Mantém a conexão MQTT
  mqttClient.loop();
  delay(10);
}
