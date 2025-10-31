# ⚽ Passa a Bola – Sistema IoT com ESP32, Node-RED e Flask

![IoT](https://img.shields.io/badge/IoT-ESP32-blue)
![MQTT](https://img.shields.io/badge/MQTT-PubSubClient-green)
![Flask](https://img.shields.io/badge/Flask-Python-orange)
![Node-RED](https://img.shields.io/badge/Node--RED-Dashboard-red)

---

## 1️⃣ Visão Geral

Este projeto implementa um **sistema de monitoramento em tempo real de passes e velocidade de um objeto**, utilizando:

* **ESP32** (simulado ou físico) como nó de borda (**Edge Computing**)
* **Node-RED** para orquestração MQTT, processamento e dashboard
* **Flask** para armazenamento simples em arquivo JSON
* **Dashboard Web** para visualização ao vivo

O objetivo é demonstrar **processamento local de dados** e visualização quase em tempo real.

---

## 2️⃣ Funcionalidades

* 📡 **Detecção de passes** via sensor ultrassônico  
* ⚡ **Simulação de velocidade** em m/s  
* 📨 **Publicação via MQTT** (`passa-a-bola/dados`)  
* 📊 **Node-RED**: recebe MQTT, envia ao Flask e atualiza dashboard  
  - Gauge de velocidade  
  - Contador de passes acumulados  
* 💾 **Persistência em `dados.json`** para consultas futuras  

---

## 3️⃣ Arquitetura e Fluxo de Dados

1. ESP32 conecta-se à rede WiFi e ao broker MQTT público (`broker.hivemq.com`)  
2. Medição do sensor ultrassônico e simulação de velocidade  
3. Payload JSON enviado via MQTT:

```json
{"passes": <número>, "velocidade": <valor>}
```

4. Node-RED:  
   * Converte JSON  
   * Envia HTTP POST ao Flask  
   * Atualiza dashboard  

5. Flask grava os dados no arquivo `dados.json` com timestamp  
6. Dashboard acessível em [http://127.0.0.1:1880/ui](http://127.0.0.1:1880/ui)

---

### 3.1 🧩 Diagrama e Prints da Arquitetura

#### 🔹 Arquitetura Geral
![Arquitetura](imgs/wokwi%201.PNG)

#### 🔹 Simulação Wokwi
| Etapa | Imagem |
|-------|--------|
| Conexão dos sensores | ![Wokwi 1](imgs/wokwi%201.PNG) |
| Publicação MQTT | ![Wokwi 2](imgs/wokwi%202.PNG) |
| Comunicação Node-RED | ![Wokwi 3](imgs/wokwi%203.PNG) |
| Teste final | ![Wokwi 4](imgs/wokwi%204.PNG) |

---

## 4️⃣ Requisitos

* **Software:** Python, Flask, Node.js, Node-RED, node-red-dashboard  
* **Biblioteca MQTT** para ESP32 (`PubSubClient`)  
* **Hardware:** ESP32 + sensor ultrassônico HC-SR04 (ou simulação via Wokwi)

---

## 5️⃣ Código do Projeto

### 🐍 Flask (`app.py`)

```python
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
```

#### 🔸 Execução Flask
![Flask Execução](imgs/py%201.PNG)
![Flask Recebendo Dados](imgs/py%202.PNG)

---

### ⚙️ ESP32 (`esp32.ino`)

```cpp
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
    } else { delay(5000); }
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
```

---

### 🧠 Node-RED

> Importar o fluxo `fluxo_passa_a_bola.json`

* Recebe MQTT → Converte JSON → POST Flask → Atualiza dashboard
* Exibe gauge de velocidade e contador de passes

#### 🔸 Fluxo Node-RED
![Fluxo Node-RED](imgs/node%20red%201.PNG)

#### 🔸 Dashboard Node-RED
![Dashboard Node-RED](imgs/node%20red%202.PNG)

---

## 6️⃣ Estrutura de Arquivos

```
sprint-4/
│
├─ imgs/
│  ├─ node red 1.PNG
│  ├─ node red 2.PNG
│  ├─ py 1.PNG
│  ├─ py 2.PNG
│  ├─ wokwi 1.PNG
│  ├─ wokwi 2.PNG
│  ├─ wokwi 3.PNG
│  └─ wokwi 4.PNG
│
├─ app.py
├─ dados.json
├─ esp32.ino
├─ fluxo_passa_a_bola.json
├─ README.md
```

---

## 7️⃣ Como Executar

### 🐍 Flask
```bash
py app.py
```

### 🧱 Node-RED
1. Abrir Node-RED → Menu → Import → JSON do fluxo  
2. Configurar broker MQTT (`broker.hivemq.com`)  
3. Deploy  
4. Acesse o dashboard: [http://127.0.0.1:1880/ui](http://127.0.0.1:1880/ui)

### ⚙️ ESP32
1. Configurar WiFi e broker MQTT  
2. Subir código no Wokwi ou placa física  
3. Monitor Serial mostra envios MQTT

---

## 8️⃣ Teste e Validação

* Flask Console: confirma recebimento dos dados  
* Node-RED Debug: mostra payload MQTT e retorno HTTP  
* Dashboard: atualiza em tempo real

---

## 9️⃣ Participantes

| Nome                                 | RM       |
| ------------------------------------ | -------- |
| Henrique de Oliveira Gomes           | RM566424 |
| Henrique Kolomyes Silveira           | RM563467 |
| Matheus Santos de Oliveira           | RM561982 |
| Vinicius Alexandre Aureliano Ribeiro | RM561606 |

---

## 🔟 Observações

* Contagem de passes real e velocidade simulada  
* Processamento local (**Edge Computing**)  
* Persistência em arquivo JSON (substituível por banco de dados)

---
