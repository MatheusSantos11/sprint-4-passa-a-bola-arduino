# Passa a Bola – Sistema IoT com ESP32, Node-RED e Flask

## 1. Visão Geral  
Este projeto implementa um sistema de monitoramento em tempo real de passes e velocidade de um objeto, utilizando:  
- ESP32 (simulado ou físico) como nó de borda (Edge Computing)  
- Node‑RED para orquestração MQTT, processamento e dashboard  
- Flask para armazenamento simples em arquivo JSON  
- Um dashboard web para visualização ao vivo dos dados  

O sistema demonstra como a Edge Computing permite processar dados localmente e disponibilizá-los para análise e visualização quase em tempo real.

---

## 2. Funcionalidades  
- Detecção de passes via sensor ultrassônico (cada vez que o objeto passa próximo do sensor)  
- Simulação de velocidade em m/s  
- Publicação via MQTT (`passa-a-bola/dados`)  
- Node‑RED recebe dados MQTT, envia para Flask e atualiza dashboard  
  - Gauge de velocidade  
  - Contador de passes acumulados  
- Persistência em `dados.json` para consultas futuras

---

## 3. Arquitetura e Fluxo de Dados  
1. ESP32 conecta-se à rede WiFi e ao broker MQTT público (`broker.hivemq.com`)  
2. Medição do sensor ultrassônico e simulação de velocidade  
3. Payload JSON enviado via MQTT:
```json
{"passes": <número>, "velocidade": <valor>}
Node‑RED recebe o MQTT, converte JSON, envia HTTP POST ao Flask e atualiza dashboard

Flask grava os dados no arquivo dados.json com timestamp

Dashboard (http://127.0.0.1:1880/ui) exibe velocidade e contagem de passes em tempo real

4. Requisitos
Python 3.x

Flask

Node.js + Node-RED + node-red-dashboard

Biblioteca MQTT para ESP32 (PubSubClient)

Hardware: ESP32 + sensor ultrassônico HC-SR04 (ou simulação via Wokwi)

5. Código do Projeto
5.1 Flask (app.py)
python
Copiar código
from flask import Flask, request, jsonify
import json, os
from datetime import datetime

app = Flask(__name__)
ARQUIVO = "dados.json"

if not os.path.exists(ARQUIVO):
    with open(ARQUIVO, "w") as f:
        json.dump([], f)

@app.route("/dados", methods=["POST"])
def receber_dados():
    dado = request.get_json()
    dado["timestamp"] = datetime.now().strftime("%Y-%m-%d %H:%M:%S")

    with open(ARQUIVO) as f:
        dados = json.load(f)
    dados.append(dado)
    with open(ARQUIVO, "w") as f:
        json.dump(dados, f, indent=2)

    return jsonify({"status": "ok", "total": len(dados)})

@app.route("/dados", methods=["GET"])
def listar_dados():
    with open(ARQUIVO) as f:
        return jsonify(json.load(f))

if __name__ == "__main__":
    app.run(host="0.0.0.0", port=5000)
5.2 ESP32 (esp32.ino)
cpp
Copiar código
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
int lastDistance = 0;

void setup_wifi() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  Serial.println("\n✅ WiFi conectado!");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  while (!client.connected()) {
    if (client.connect("PassaBolaESP32")) {
      Serial.println("✅ Conectado ao MQTT");
    } else {
      delay(5000);
    }
  }
}

void setup() {
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(ledPin, OUTPUT);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
}

void loop() {
  if (!client.connected()) reconnect();
  client.loop();

  digitalWrite(trigPin, LOW); delayMicroseconds(2);
  digitalWrite(trigPin, HIGH); delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  duration = pulseIn(echoPin, HIGH);
  distanceCm = duration * 0.034 / 2;

  if (distanceCm < 50 && lastDistance >= 50) {
    passes++;
    digitalWrite(ledPin, HIGH); delay(200); digitalWrite(ledPin, LOW);
  }
  lastDistance = distanceCm;

  float velocidade = random(60, 120) / 10.0;
  String payload = "{\"passes\":" + String(passes) + ",\"velocidade\":" + String(velocidade) + "}";
  client.publish("passa-a-bola/dados", payload.c_str());
  Serial.println("📤 Enviado MQTT: " + payload);
  delay(5000);
}
5.3 Node-RED

[
    {
        "id": "passaabola_flow",
        "type": "tab",
        "label": "Passa a Bola Dashboard",
        "disabled": false,
        "info": ""
    },
    {
        "id": "mqtt_in",
        "type": "mqtt in",
        "z": "passaabola_flow",
        "name": "Receber do ESP32",
        "topic": "passa-a-bola/dados",
        "qos": "0",
        "datatype": "auto",
        "broker": "mqtt_broker",
        "x": 160,
        "y": 100,
        "wires": [["debug_mqtt","json_parse"]]
    },
    {
        "id": "json_parse",
        "type": "json",
        "z": "passaabola_flow",
        "name": "Converter JSON",
        "property": "payload",
        "action": "obj",
        "x": 380,
        "y": 100,
        "wires": [["http_post","function_dashboard"]]
    },
    {
        "id": "http_post",
        "type": "http request",
        "z": "passaabola_flow",
        "name": "Enviar ao Flask",
        "method": "POST",
        "ret": "txt",
        "url": "http://127.0.0.1:5000/dados",
        "x": 610,
        "y": 60,
        "wires": [["debug_flask"]]
    },
    {
        "id": "function_dashboard",
        "type": "function",
        "z": "passaabola_flow",
        "name": "Preparar para Dashboard",
        "func": "msg.payload = {\n    velocidade: msg.payload.velocidade,\n    passes: msg.payload.passes\n};\nreturn msg;",
        "outputs": 1,
        "x": 610,
        "y": 140,
        "wires": [["gauge_velocidade","text_passes"]]
    },
    {
        "id": "gauge_velocidade",
        "type": "ui_gauge",
        "z": "passaabola_flow",
        "name": "Velocidade (m/s)",
        "group": "ui_group",
        "order": 1,
        "width": 6,
        "height": 4,
        "gtype": "gage",
        "title": "Velocidade (m/s)",
        "label": "m/s",
        "format": "{{msg.payload.velocidade}}",
        "min": 0,
        "max": 15,
        "colors": ["#00b500","#e6e600","#ca3838"],
        "seg1": 5,
        "seg2": 10,
        "x": 880,
        "y": 80,
        "wires": []
    },
    {
        "id": "text_passes",
        "type": "ui_text",
        "z": "passaabola_flow",
        "group": "ui_group",
        "order": 2,
        "width": 6,
        "height": 1,
        "name": "Qtd de Passes",
        "label": "Passes:",
        "format": "{{msg.payload.passes}}",
        "layout": "row-spread",
        "x": 880,
        "y": 140,
        "wires": []
    },
    {
        "id": "debug_mqtt",
        "type": "debug",
        "z": "passaabola_flow",
        "name": "📥 Recebido MQTT",
        "active": true,
        "tosidebar": true,
        "complete": "payload",
        "x": 410,
        "y": 40,
        "wires": []
    },
    {
        "id": "debug_flask",
        "type": "debug",
        "z": "passaabola_flow",
        "name": "📤 Retorno Flask",
        "active": true,
        "tosidebar": true,
        "complete": "payload",
        "x": 830,
        "y": 40,
        "wires": []
    },
    {
        "id": "mqtt_broker",
        "type": "mqtt-broker",
        "name": "HiveMQ Público",
        "broker": "broker.hivemq.com",
        "port": "1883",
        "protocolVersion": "4",
        "keepalive": "60",
        "cleansession": true
    },
    {
        "id": "ui_group",
        "type": "ui_group",
        "name": "Painel Passa a Bola",
        "tab": "ui_tab",
        "order": 1,
        "width": 6,
        "collapse": false
    },
    {
        "id": "ui_tab",
        "type": "ui_tab",
        "name": "Dashboard Passa a Bola",
        "icon": "dashboard"
    }
]


6. Estrutura de Arquivos
sprint-4/
│
├─ app.py
├─ dados.json
├─ node-red-flow.json
├─ esp32.ino
├─ README.md
└─ (outros arquivos)
7. Como Executar
7.1 Flask
bash
Copiar código
py app.py
7.2 Node-RED
Abra Node-RED → Menu → Import → JSON do fluxo

Configure broker MQTT → broker.hivemq.com

Deploy

Dashboard: http://127.0.0.1:1880/ui

7.3 ESP32
Configure WiFi e broker MQTT

Suba o código

Serial Monitor mostra envios MQTT

8. Teste e Validação
Flask console: verifica recebimento (📩 Recebido: {...})

Node-RED debug: mostra payload do MQTT e retorno do Flask

Dashboard: gauge de velocidade e contador de passes atualizando em tempo real

9. Participantes
Henrique Kolomyes Silveira | RM563467

Matheus Santos de Oliveira | RM561982


10. Observações
Contagem de passes real, velocidade simulada

Processamento local (Edge Computing)

Arquivo JSON para persistência, recomendado banco de dados em produção
