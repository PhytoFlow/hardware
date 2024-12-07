#include "DHT.h"
#include <OneWire.h>
#include <DallasTemperature.h>

#define DHTPIN 2
#define DHTTYPE DHT22

DHT dht(DHTPIN, DHTTYPE);

const int pino_luz = A0;
const int pino_umidade_solo = A1;
const int pino_UV = A2;
const int pino_temperatura_solo = 3;

const int soloSeco = 800;
const int soloUmido = 300;

float temperatura = 0.0;
float umidade = 0.0;

OneWire oneWire(pino_temperatura_solo);
DallasTemperature sensors(&oneWire);

void setup() {
  pinMode(pino_luz, INPUT);
  pinMode(pino_umidade_solo, INPUT);
  pinMode(pino_UV, INPUT);

  dht.begin();
  sensors.begin();

  Serial.begin(9600);
}

void loop() {
  temperatura = dht.readTemperature();
  umidade = dht.readHumidity();

  sensors.requestTemperatures();
  float temperaturaSolo = sensors.getTempCByIndex(0);

  int qntLuz = analogRead(pino_luz);
  int leituraUmidadeSolo = analogRead(pino_umidade_solo);
  int qntLuzUV = analogRead(pino_UV);

  float umidadeSoloPercentual = map(leituraUmidadeSolo, soloSeco, soloUmido, 0, 100);
  umidadeSoloPercentual = constrain(umidadeSoloPercentual, 0, 100);

  Serial.print("Temperatura: ");
  Serial.print(temperatura);
  Serial.print(", Umidade: ");
  Serial.print(umidade);
  Serial.print(", Umidade Solo: ");
  Serial.print(umidadeSoloPercentual);
  Serial.print(", Intensidade UV: ");
  Serial.print(qntLuzUV);
  Serial.print(", Temperatura Solo: ");
  Serial.println(temperaturaSolo);

  delay(1000);
}
