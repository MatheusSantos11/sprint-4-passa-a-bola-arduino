from flask import Flask, request, jsonify
import json, os
from datetime import datetime

app = Flask(__name__)
ARQUIVO = "dados.json"

# Cria o arquivo se não existir
if not os.path.exists(ARQUIVO):
    with open(ARQUIVO, "w") as f:
        json.dump([], f)

@app.route("/dados", methods=["POST"])
def receber_dados():
    try:
        dado = request.get_json()
        print("📩 Recebido:", dado)

        dado["timestamp"] = datetime.now().strftime("%Y-%m-%d %H:%M:%S")

        with open(ARQUIVO) as f:
            dados = json.load(f)

        dados.append(dado)

        with open(ARQUIVO, "w") as f:
            json.dump(dados, f, indent=2)

        return jsonify({"status": "sucesso", "total": len(dados)}), 200

    except Exception as e:
        print("❌ Erro ao receber dado:", e)
        return jsonify({"status": "erro", "detalhe": str(e)}), 400


@app.route("/dados", methods=["GET"])
def listar_dados():
    with open(ARQUIVO) as f:
        return jsonify(json.load(f))

if __name__ == "__main__":
    print("🚀 Flask rodando em http://127.0.0.1:5000 e http://SEU_IP_LOCAL:5000")
    app.run(host="0.0.0.0", port=5000)

