from flask import Flask, jsonify
from flask_cors import CORS

app = Flask(__name__)
CORS(app) 

GEMINI_API_KEY = "AIzaSyABt0qNt17GRH6fzacLWm0gQxR4y2sCrRo"

@app.route('/api/keys', methods=['GET'])
def get_keys():
    return jsonify({
        'gemini_api_key': GEMINI_API_KEY
    })

if __name__ == '__main__':
    app.run(host='127.0.0.1', port=5000)
