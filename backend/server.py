from flask import Flask, jsonify
from flask_cors import CORS

app = Flask(__name__)
CORS(app) 

IMGUR_CLIENT_ID = "e2c17c691e83145"
OPENAI_API_KEY = "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"

@app.route('/api/keys', methods=['GET'])
def get_keys():
    return jsonify({
        'imgur_client_id': IMGUR_CLIENT_ID,
        'openai_api_key': OPENAI_API_KEY
    })

if __name__ == '__main__':
    app.run(host='127.0.0.1', port=5000)
