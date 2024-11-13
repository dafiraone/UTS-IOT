#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <WebServer.h>

// Konfigurasi WiFi
const char* ssid = "iDfr1";
const char* password = "reinamishima";

// Konfigurasi MQTT
const char* mqtt_server = "broker.hivemq.com";
const char* mqtt_topic = "152022003_UTS";

WiFiClient espClient;
PubSubClient mqttClient(espClient);
WebServer server(80);

// Konfigurasi DHT
#define DHTPIN 4
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

// Konfigurasi pin LED, buzzer, relay, dan LDR
#define LED_HIJ 5
#define LED_KUN 18
#define LED_MER 19
#define BUZZER 21
#define RELAY 22
#define LDR_PIN 34

// Konfigurasi pin relay
#define RELAY 22

// Fungsi untuk menghubungkan ke WiFi
void setup_wifi() {
  delay(10);
  Serial.println("Menghubungkan ke WiFi...");
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("\nWiFi tersambung");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}

// Fungsi untuk menyambung ulang ke MQTT jika koneksi terputus
void reconnect_mqtt() {
  while (!mqttClient.connected()) {
    Serial.print("Menghubungkan ke MQTT...");
    if (mqttClient.connect("ESP32Client")) {
      Serial.println("MQTT tersambung");
    } else {
      Serial.print("Gagal, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" mencoba lagi dalam 5 detik");
      delay(5000);
    }
  }
}

// Fungsi untuk menangani kontrol relay melalui HTTP
void handleRelayOn() {
  digitalWrite(RELAY, LOW);
}

void handleRelayOff() {
  digitalWrite(RELAY, HIGH);
}

void handleNotFound() {
  server.send(404, "text/plain", "Not Found");
}

void setup() {
  Serial.begin(115200);

  // Hubungkan ke WiFi
  setup_wifi();

  // Setup MQTT
  mqttClient.setServer(mqtt_server, 1883);

  // Setup API server
  server.on("/relay/on", handleRelayOn);   // API untuk menyalakan relay
  server.on("/relay/off", handleRelayOff); // API untuk mematikan relay
  server.onNotFound(handleNotFound);       // Rute jika URL tidak ditemukan
  server.begin();

  // Konfigurasi DHT dan relay
  dht.begin();
  pinMode(RELAY, OUTPUT);
  digitalWrite(RELAY, LOW); // Pastikan relay mati saat start

  pinMode(LED_HIJ, OUTPUT);
  pinMode(LED_KUN, OUTPUT);
  pinMode(LED_MER, OUTPUT);
  pinMode(BUZZER, OUTPUT);
  pinMode(RELAY, OUTPUT);
  pinMode(LDR_PIN, INPUT);
}

void loop() {
  // Periksa koneksi MQTT
  if (!mqttClient.connected()) {
    reconnect_mqtt();
  }
  mqttClient.loop();

  // Jalankan server API
  server.handleClient();

  // Baca data dari sensor
  float suhu = dht.readTemperature();
  float kelembapan = dht.readHumidity();
  int ldrValue = analogRead(34); // Baca nilai dari LDR (gunakan ADC)

  // Validasi data
  if (isnan(suhu) || isnan(kelembapan)) {
    Serial.println("Gagal membaca data dari sensor DHT");
    return;
  }

  if (suhu > 35) {
    digitalWrite(LED_HIJ, LOW);
    digitalWrite(LED_KUN, LOW);
    digitalWrite(LED_MER, HIGH);
    digitalWrite(BUZZER, HIGH);
  } else if (suhu >= 30 && suhu <= 35) {
    digitalWrite(LED_HIJ, LOW);
    digitalWrite(LED_KUN, HIGH);
    digitalWrite(LED_MER, LOW);
    digitalWrite(BUZZER, LOW);
  } else {
    digitalWrite(LED_HIJ, HIGH);
    digitalWrite(LED_KUN, LOW);
    digitalWrite(LED_MER, LOW);
    digitalWrite(BUZZER, LOW);
  }

  // Kirim data sensor ke MQTT
  String payload = "{\"suhu\": " + String(suhu) + 
                   ", \"kelembapan\": " + String(kelembapan) + 
                   ", \"kecerahan\": " + String(ldrValue) + "}";
  mqttClient.publish(mqtt_topic, payload.c_str());
  Serial.println("Data sensor dikirim ke MQTT: " + payload);

  delay(5000); // Kirim data setiap detik
}