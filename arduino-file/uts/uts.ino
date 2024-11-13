#include <DHT.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include "PubSubClient.h"

// Pin sensor
const int DHTPIN = 15;        // Pin DHT22 untuk suhu dan kelembapan
const int DHTTYPE = DHT22;    // Tipe sensor DHT yang digunakan
const int TURBIDITY_PIN = 34; // Pin analog untuk simulasi sensor kekeruhan
const int PH_PIN = 35;        // Pin analog untuk simulasi sensor pH

// Pin kontrol perangkat
const int LED_GREEN = 5;      // Pin untuk LED Hijau
const int LED_YELLOW = 17;    // Pin untuk LED Kuning
const int LED_RED = 4;        // Pin untuk LED Merah
const int RELAY_PUMP = 27;    // Pin untuk relay pompa
const int BUZZER = 2;         // Pin untuk buzzer

// WiFi settings
const char* ssid = "nayla";               // Ganti dengan SSID jaringan Anda
const char* password = "outlander";      // Ganti dengan password WiFi Anda

// MQTT Broker settings
const char* mqtt_server = "1sampai8";  // Ganti dengan IP atau alamat broker MQTT
const int mqtt_port = 1883;                    // Port broker MQTT
const char* pub_lampu = "insanlamp";
const char* pub_relay = "insanrelay";
const char* pub_buzzer = "insanbuzzer";
const char* sub_suhu = "insantemperature";
const char* sub_humi = "insanhumidity";
const char* sub_turbidity = "insanturbidity";
const char* sub_ph = "insanph";

// Alamat URL Flask server
const char* flask_server_url = "http://192.168.1.18:5000/data";  // Ganti <NGROK_URL> dengan URL ngrok yang digunakan

WiFiClient espClient;
PubSubClient client(espClient);

DHT dht(DHTPIN, DHTTYPE);

// Variabel untuk menyimpan nilai suhu dan kelembapan
float suhu = -1;
float humidity = -1;

void setup() {
  Serial.begin(115200);

  // Setup untuk sensor
  dht.begin();
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_YELLOW, OUTPUT);
  pinMode(LED_RED, OUTPUT);
  pinMode(RELAY_PUMP, OUTPUT);
  pinMode(BUZZER, OUTPUT);

  digitalWrite(LED_GREEN, LOW);
  digitalWrite(LED_YELLOW, LOW);
  digitalWrite(LED_RED, LOW);
  digitalWrite(RELAY_PUMP, LOW);
  digitalWrite(BUZZER, LOW);

  // Connect to WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Connected to WiFi");

  // Connect to MQTT Broker
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(mqtt_callback);

  // Connect to MQTT broker
  connectToMQTT();
}

void loop() {
  if (!client.connected()) {
    connectToMQTT();
  }
  client.loop();

  delay(2000);  // Delay sebelum pembacaan data berikutnya
}

void connectToMQTT() {
  while (!client.connected()) {
    Serial.print("Connecting to MQTT...");
    String clientId = "ESP32Client-";
    clientId += String(random(0xffff), HEX);

    if (client.connect(clientId.c_str())) {
      Serial.println("Connected to MQTT broker");
      // Subscribe to sensor data topics to receive sensor values
      client.subscribe(sub_suhu);
      client.subscribe(sub_humi);
      client.subscribe(sub_turbidity);
      client.subscribe(sub_ph);
    } else {
      Serial.print("Failed to connect, rc=");
      Serial.print(client.state());
      Serial.println(" Try again in 5 seconds");
      delay(5000);
    }
  }
}

void mqtt_callback(char* topic, byte* payload, unsigned int length) {
  String message = "";
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  if (String(topic) == sub_suhu) {
    Serial.print("Received Suhu: ");
    Serial.println(message);

    suhu = message.toFloat();
    String lampuStatus;

    if (suhu < 30) {
      digitalWrite(LED_GREEN, HIGH);
      digitalWrite(LED_YELLOW, LOW);
      digitalWrite(LED_RED, LOW);
      lampuStatus = "{\"green\": \"ON\"}";
    } else if (suhu >= 30 && suhu <= 35) {
      digitalWrite(LED_GREEN, LOW);
      digitalWrite(LED_YELLOW, HIGH);
      digitalWrite(LED_RED, LOW);
      lampuStatus = "{\"yellow\": \"ON\"}";
    } else {
      digitalWrite(LED_GREEN, LOW);
      digitalWrite(LED_YELLOW, LOW);
      digitalWrite(LED_RED, HIGH);
      digitalWrite(BUZZER, HIGH);
      lampuStatus = "{\"red\": \"ON\"}";
    }
    client.publish(pub_lampu, lampuStatus.c_str());

    // Kirim suhu dan kelembapan ke Flask jika kelembapan telah diterima
    if (humidity != -1) {
      sendToFlask(suhu, humidity);
    }
  }

  if (String(topic) == sub_humi) {
    Serial.print("Received Kelembapan: ");
    Serial.println(message);

    humidity = message.toFloat();

    if (humidity < 50) {
      if (digitalRead(RELAY_PUMP) == LOW) {
        digitalWrite(RELAY_PUMP, HIGH);
        client.publish(pub_relay, "Pompa ON");
      }
      if (digitalRead(BUZZER) == LOW) {
        digitalWrite(BUZZER, HIGH);
        client.publish(pub_buzzer, "Buzzer ON");
      }
    } else {
      if (digitalRead(RELAY_PUMP) == HIGH) {
        digitalWrite(RELAY_PUMP, LOW);
        client.publish(pub_relay, "Pompa OFF");
      }
      if (digitalRead(BUZZER) == HIGH) {
        digitalWrite(BUZZER, LOW);
        client.publish(pub_buzzer, "Buzzer OFF");
      }
    }

    // Kirim suhu dan kelembapan ke Flask jika suhu telah diterima
    if (suhu != -1) {
      sendToFlask(suhu, humidity);
    }
  }
}

void sendToFlask(float suhu, float humidity) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(flask_server_url);

    // JSON data sesuai dengan yang diminta
    String jsonData = "{\"suhumax\":23,\"suhumin\":38,\"suhurata2\":28,\"nilaisuhuhumid\":[{\"id\":1,\"suhu\":"
                      + String(suhu) + ",\"humid\":" + String(humidity) 
                      + ",\"kecerahan\":200,\"timestamp\":\"2024-11-12T08:00:00\"}],\"month_year\":\"11-2024\"}";

    http.addHeader("Content-Type", "application/json");
    http.setTimeout(5000); // Mengatur timeout menjadi 5 detik

    int httpResponseCode = http.POST(jsonData);
    if (httpResponseCode > 0) {
      String response = http.getString();
      Serial.println("Response: " + response);
    } else {
      Serial.print("Failed to send POST, HTTP Code: ");
      Serial.println(httpResponseCode);
    }

    http.end();
  } else {
    Serial.println("Error in WiFi connection");
  }
}
