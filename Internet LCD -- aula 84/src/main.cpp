#include <Arduino.h>
#include <PubSubClient.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include <ezTime.h>
#include <LiquidCrystal_I2C.h>
#include "internet.h"


 byte relogio[] = { B00000, B01110, B10001, B10111, B10101, B01110, B00000, B00000};
 byte gota[] = { B00000, B00100, B01010, B10001, B10001, B01110, B00000, B00000 };
 byte sol[] = { B00000, B10101, B01010, B10001, B01010, B10101, B00000, B00000 };
#define pinBotao 0
#define pinLed 2

WiFiClient espClient;
PubSubClient client(espClient);
Timezone tempo;
LiquidCrystal_I2C lcd(0x27, 20, 4);

bool envioMqtt = false;
float tempoEspera = 500; // velocidade em ms
float tempoEnvio = 5000; // velocidade em ms
float umid, temp;

const char *mqtt_server = "broker.hivemq.com";
const int mqtt_port = 1883;
const char *mqtt_id = "esp32-senai134-pedroaugusto";
const char *mqtt_topic_sub = "senai134/mesa05/esp_publicando";
const char *mqtt_topic_pub = "senai134/mesa05/esp_publicand";

void callback(char *, byte *, unsigned int);
void mqttConnect();

void setup()
{
  Serial.begin(9600);
  conectaWiFi();
  pinMode(pinLed, OUTPUT);

  // * MQTT
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);

  // * ezTime
  waitForSync();
  tempo.setLocation("America/Sao_Paulo");

  // * LCD
  lcd.init();
  lcd.backlight();
  lcd.setCursor(18,0);
  lcd.createChar(0,sol);
  lcd.write(0);
  lcd.setCursor(18,1);
  lcd.createChar(1,gota);
  lcd.write(1);
  lcd.setCursor(18,2);
  lcd.createChar(3,relogio);
  lcd.write(3);
}

void loop()
{
  checkWiFi();

  if (!client.connected()) mqttConnect();
  client.loop();

  static unsigned long tempoAnteriorMsg = 0;
  unsigned long tempoAtual = millis();

  JsonDocument doc;
  String controleTempUmid = "";

  if (tempoAtual - tempoAnteriorMsg > tempoEnvio)
  {
    doc.clear();
    doc["temperatura"] = temp;
    doc["umidade"] = umid;

    // Temperatura
    lcd.setCursor(0, 0);
    lcd.print("Temperatura: ");
    if (isnan(temp))
    {
      lcd.print("---");
    }
    else
    {
      lcd.print(temp);
      lcd.print("C");
    }

    // Umidade
    lcd.setCursor(0, 1);
    lcd.print("Umidade: ");
    if (isnan(umid))
    {
      lcd.print("---");
    }
    else
    {
      lcd.print(umid);
      lcd.print("%");
    }

    // Data
    lcd.setCursor(0, 3);
    lcd.print("Data: ");
    lcd.print(tempo.dateTime("d/m/Y"));

    // Serializar depois de preencher o JSON
    serializeJson(doc, controleTempUmid);
    Serial.println(controleTempUmid);
    client.publish(mqtt_topic_pub, controleTempUmid.c_str());

    tempoAnteriorMsg = tempoAtual;
  }
     // Hora
    lcd.setCursor(0, 2);
    lcd.print("Hora: ");
    lcd.print(tempo.dateTime("H:i:s"));

}

void callback(char *topic, byte *payLoad, unsigned int Length)
{
  Serial.printf("Mensagem recebida em %s: ", topic, "/n");

  String controleTempUmid = "";
  for (unsigned int i = 0; i < Length; i++)
  {
    char c = (char)payLoad[i];
    controleTempUmid += c;
  }

  JsonDocument doc;
  deserializeJson(doc, controleTempUmid.c_str());

  if (!doc["temperatura"].isNull())
  {
    temp = doc["temperatura"];
  }
  if (!doc["umidade"].isNull())
  {
    umid = doc["umidade"];
  }
  if (!doc["tempoEnvio"].isNull())
  {
    tempoEnvio = doc["tempoEnvio"];
  }
};
void mqttConnect()
{
  while (!client.connected())
  {
    Serial.println("Conectando ao MQTT...");

    if (client.connect(mqtt_id))
    {
      Serial.println("Conectado com sucesso");
      client.subscribe(mqtt_topic_sub);
    }
    else
    {
      Serial.print("falha, rc=");
      Serial.println(client.state());
      Serial.println("tentando novamente em 5 segundos");
      delay(5000);
    }
  }
}