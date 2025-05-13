

#include <WiFiEspAT.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>
#include "Alarm.h"
#include "PorteAutomatique.h"


#define STUDENT_ID "10"
#define TOPIC_PUBLISH "etd/" STUDENT_ID "/data"
#define TOPIC_SUBSCRIBE "etd/" STUDENT_ID "/data"
#define AT_BAUD_RATE 115200
#define DHTPIN 8
#define DHTTYPE DHT11


#define TRIGGER_PIN 9
#define ECHO_PIN 10
#define RED_PIN 3
#define BLUE_PIN 5
#define BUZZER 4
#define STEP1 31
#define STEP2 33
#define STEP3 35
#define STEP4 37


LiquidCrystal_I2C lcd(0x27, 16, 2);
WiFiClient wifiClient;
PubSubClient client(wifiClient);
DHT dht(DHTPIN, DHTTYPE);

float distance = 0;
String couleur = "#redblue";
String lcdLine1 = "";
String lcdLine2 = "";
bool lcdDirty = false;
bool moteurActif = false;

Alarm alarm(RED_PIN, BLUE_PIN, BUZZER, &distance);
PorteAutomatique porte(STEP1, STEP2, STEP3, STEP4, distance);

unsigned long lastMQTTSend = 0;
const unsigned long mqttInterval = 2500;
unsigned long lastLCDSend = 0;
const unsigned long lcdInterval = 1100;

StaticJsonDocument<512> lastDoc;

void reconnectMQTT();

float lireDistance() {
  static float lastValid = 999;
  digitalWrite(TRIGGER_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIGGER_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIGGER_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH, 30000);
  float newDist = duration * 0.034 / 2.0;

  if (duration == 0 || newDist <= 0 || newDist > 400) {
    return lastValid;
  }

  lastValid = newDist;
  return newDist;
}

bool hasChanged(const JsonDocument& a, const JsonDocument& b) {
  return memcmp(&a, &b, sizeof(JsonDocument)) != 0;
}

void envoyerEtat() {
  static unsigned long startTime = millis();
  unsigned long uptime = (millis() - startTime) / 1000;

  float angle = porte.getAngle();
  int motor = moteurActif ? 1 : 0;

  //distance = lireDistance();
  float temp = dht.readTemperature();
  float hum = dht.readHumidity();

  StaticJsonDocument<512> doc;
  doc["name"] = "Baye Ousseynou Diagne";
  doc["number"] = 2419796;
  doc["uptime"] = uptime;
  doc["motor"] = motor;
  doc["color"] = couleur;
  doc["line1"] = lcdLine1;
  doc["line2"] = lcdLine2;

  if (!isnan(distance)) doc["dist"].set(distance);
  else doc["dist"].set(nullptr);

  if (!isnan(angle)) doc["angle"].set(angle);
  else doc["angle"].set(nullptr);

  if (!isnan(temp)) doc["temp"].set(temp);
  else doc["temp"].set(nullptr);

  if (!isnan(hum)) doc["hum"].set(hum);
  else doc["hum"].set(nullptr);

  if (!hasChanged(doc, lastDoc)) return;

  char message[512];
  serializeJson(doc, message, sizeof(message));

  Serial.print("Envoi MQTT : ");
  Serial.println(message);
  client.publish(TOPIC_PUBLISH, message);

  lastDoc = doc;
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message reçu sur ");
  Serial.println(topic);

  char message[256];
  if (length >= sizeof(message)) length = sizeof(message) - 1;
  memcpy(message, payload, length);
  message[length] = '\0';

  Serial.print("Contenu : ");
  Serial.println(message);

  StaticJsonDocument<256> doc;
  DeserializationError error = deserializeJson(doc, message);
  if (error) {
    Serial.print("Erreur JSON : ");
    Serial.println(error.c_str());
    return;
  }

  if (doc.containsKey("color")) {
    couleur = doc["color"].as<String>();
    Serial.print(" Couleur reçue : ");
    Serial.println(couleur);
  }

  if (doc.containsKey("motor")) {
    int motorValue = doc["motor"].as<int>();
    moteurActif = (motorValue == 1);

    if (moteurActif) {
      porte.setAngleOuvert(180);
      porte.update();  
      Serial.println(" Moteur activé → ouverture à 180°");
    } else {
      
      Serial.println(" Moteur désactivé → moteur figé");
    }
  }



  if (doc.containsKey("line1")) {
    lcdLine1 = doc["line1"].as<String>();
    lcdDirty = true;
    Serial.print("LCD ligne 1 : ");
    Serial.println(lcdLine1);
  }

  if (doc.containsKey("line2")) {
    lcdLine2 = doc["line2"].as<String>();
    lcdDirty = true;
    Serial.print(" LCD ligne 2 : ");
    Serial.println(lcdLine2);
  }

  if (doc.containsKey("alm")) {
    String val = doc["alm"].as<String>();
    if (val == "ON") alarm.activateAlarm();
    else if (val == "OFF") alarm.deactivateAlarm();
    Serial.println(" Alarme : " + val);
  }
}

void reconnectMQTT() {
  while (!client.connected()) {
    Serial.print("Connexion au serveur MQTT...");
    if (client.connect("arduinoClient", "etdshawi", "shawi123")) {
      Serial.println(" connectée !");
      String topicData = "etd/" STUDENT_ID "/data";
      client.subscribe(topicData.c_str());
      Serial.print("Abonné au topic : ");
      Serial.println(topicData);
    } else {
      Serial.print(" échec, code : ");
      Serial.println(client.state());
      delay(1000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(TRIGGER_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(RED_PIN, OUTPUT);
  pinMode(BLUE_PIN, OUTPUT);
  pinMode(BUZZER, OUTPUT);

  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Demarrage...");
  delay(1500);
  lcd.clear();

  porte.setPasParTour(2048);
  porte.setAngleOuvert(180);
  porte.setAngleFerme(0);
  porte.setDistanceOuverture(20);
  porte.setDistanceFermeture(30);

  dht.begin();

  Serial1.begin(AT_BAUD_RATE);
  WiFi.init(&Serial1);

  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Erreur : module WiFi introuvable !");
    while (true)
      ;
  }

  WiFi.disconnect();
  WiFi.setPersistent();
  WiFi.endAP();

  Serial.println("Connexion à : TechniquesInformatique-Etudiant");
  int status = WiFi.begin("TechniquesInformatique-Etudiant", "shawi123");
  if (status == WL_CONNECTED) {
    Serial.println("Connecté au WiFi.");
  } else {
    Serial.println("Connexion WiFi échouée.");
    while (true)
      ;
  }

  client.setServer("216.128.180.194", 1883);
  client.setCallback(callback);
}

void loop() {
  if (!client.connected()) reconnectMQTT();
  client.loop();

  distance = lireDistance();
  alarm.update();

  if (moteurActif) porte.update();

  static unsigned long lastBlink = 0;
  static bool ledState = false;
  if (distance > 0 && distance <= 20) {
    if (millis() - lastBlink > 300) {
      lastBlink = millis();
      ledState = !ledState;
      analogWrite(RED_PIN, ledState ? 255 : 0);
      analogWrite(BLUE_PIN, ledState ? 0 : 255);
    }
  } else {
    analogWrite(RED_PIN, 0);
    analogWrite(BLUE_PIN, 0);
  }

  unsigned long currentMillis = millis();
  if (currentMillis - lastMQTTSend >= mqttInterval) {
    lastMQTTSend = currentMillis;
    envoyerEtat();
  }

  if (currentMillis - lastLCDSend >= lcdInterval) {
    lastLCDSend = currentMillis;

    
    lcdLine1 = porte.getEtatTexte();
    lcdLine2 = String(distance, 1) + " cm";

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(lcdLine1.substring(0, 16));
    lcd.setCursor(0, 1);
    lcd.print(lcdLine2.substring(0, 16));
  }
}