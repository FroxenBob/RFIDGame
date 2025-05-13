from flask import Flask, render_template, jsonify
import requests
from datetime import datetime

app = Flask(__name__)

devices = [
    "http://172.20.10.10/json",
]

def fetch_all_data():
    all_scans = []
    for ip in devices:
        try:
            res = requests.get(ip, timeout=5)
            scans = res.json()
            for scan in scans:
                if 'uid' in scan and 'time' in scan:
                    scan['source'] = ip
                    all_scans.append(scan)
                else:
                    print(f" Skipped malformed entry from {ip}: {scan}")
        except Exception as e:
            print(f" Could not fetch from {ip}: {e}")
    return all_scans

def safe_time(entry):
    try:
        return datetime.strptime(entry['time'], "%Y-%m-%d %H:%M:%S")
    except Exception as e:
        print(f" Skipping bad timestamp: {entry.get('time')}")
        return datetime.max

@app.route("/")
def index():
    data = fetch_all_data()
    data.sort(key=safe_time)
    return render_template("index.html", data=data)

@app.route("/data")
def get_data():
    data = fetch_all_data()
    if not data:
        return jsonify([])  # avoid error in frontend
    data.sort(key=safe_time)
    return jsonify(data)

if __name__ == "__main__":
    app.run(debug=True)