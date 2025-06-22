from flask import Flask, jsonify
from flask_cors import CORS

app = Flask(__name__)
CORS(app) 

IMGUR_CLIENT_ID = "e2c17c691e83145"
OPENAI_API_KEY = "YOUR_OPENAsk-proj-pAz0e8ychHwAQapv7UOYqbJSS6CxIOsxhHI7_-GmznCkxR3lKEiJEDO15MIAmmt24djIsgZAq3T3BlbkFJZYWZag92824jKkvG8xgciXBg5BBCYar4yRIfomSJ9oJ4yST5uqBRwBW_YnxL6Krendb5bSNVUAI_API_KEY"

@app.route('/api/keys', methods=['GET'])
def get_keys():
    return jsonify({
        'imgur_client_id': IMGUR_CLIENT_ID,
        'openai_api_key': OPENAI_API_KEY
    })

if __name__ == '__main__':
    app.run(host='127.0.0.1', port=5000)
