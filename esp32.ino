#include <WiFi.h>
#include <PubSubClient.h>

const char* ssid = "Wokwi-GUEST";
const char* password = "";
const char* mqtt_server = "broker.hivemq.com";

WiFiClient espClient;
PubSubClient client(espClient);

const int trigPin = 5;
const int echoPin = 18;
const int ledPin  = 2;

long duration;
float distanceCm;
int passes = 0;
int lastDistance = 0;  // para evitar contar múltiplos passes seguidos

void setup_wifi() {
  Serial.print("Conectando ao WiFi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n✅ WiFi conectado!");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Conectando ao MQTT...");
    if (client.connect("PassaBolaESP32")) {
      Serial.println("✅ Conectado ao MQTT");
    } else {
      Serial.print("Falhou, rc=");
      Serial.print(client.state());
      Serial.println(" Tentando em 5s...");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(ledPin, OUTPUT);

  setup_wifi();
  client.setServer(mqtt_server, 1883);
}

void loop() {
  if (!client.connected()) reconnect();
  client.loop();

  // Medição de distância
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  duration = pulseIn(echoPin, HIGH);
  distanceCm = duration * 0.034 / 2;

  // Contagem de passes (apenas se a distância mudou de >50 para <50)
  if (distanceCm < 50 && lastDistance >= 50) {
    passes++;
    digitalWrite(ledPin, HIGH);
    delay(200);
    digitalWrite(ledPin, LOW);
  }
  lastDistance = distanceCm;

  // Velocidade simulada
  float velocidade = random(60, 120) / 10.0;

  // Monta payload JSON
  String payload = "{\"passes\":" + String(passes) +
                   ",\"velocidade\":" + String(velocidade) + "}";

  client.publish("passa-a-bola/dados", payload.c_str());
  Serial.println("📤 Enviado MQTT: " + payload);

  delay(5000);
}
