#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <LittleFS.h>
#include <MFRC522v2.h>
#include <MFRC522DriverSPI.h>
#include <MFRC522DriverPinSimple.h>
#include <MFRC522Debug.h>
#include <SPI.h>
#include <time.h>
#include <TZ.h> 
#include <map>

//saved nales
std::map<String, String> uidToName = {
  {"4:15:35:82:db:14:90", "Mikhail"},
  {"72:e9:fa:3", "Bob"},
};

// Wi-Fi access parameters
const char* ssid = "Iphone Misha";
const char* password = "123iot123iot";

// Set static IP address
//IPAddress local_IP(172, 20, 10, 50);  
//IPAddress gateway(172, 20, 10, 1);     
//IPAddress subnet(255, 255, 255, 0);

ESP8266WebServer server(80);

// Learn more about using SPI/I2C or check the pin assigment for your board: https://github.com/OSSLibraries/Arduino_MFRC522v2#pin-layout
MFRC522DriverPinSimple ss_pin(15);
MFRC522DriverSPI driver{ss_pin}; // Create SPI driver
//MFRC522DriverI2C driver{};     // Create I2C driver
MFRC522 mfrc522{driver};         // Create MFRC522 instance

//webpage 
void handleRoot() {
  String page = R"rawliteral(
  <!DOCTYPE html>
  <html>
  <head>
    <meta charset="UTF-8">
    <title>RFID Scan Log</title>
    <style>
      body {
        font-family: Arial, sans-serif;
        background: #f4f4f4;
        margin: 2em;
        color: #333;
      }
      h2 {
        text-align: center;
      }
      table {
        width: 80%;
        border-collapse: collapse;
        margin: 20px 0;
        background: white;
      }
      th, td {
        padding: 12px;
        border-bottom: 1px solid #ccc;
        text-align: left;
      }
      th {
        background: #4CAF50;
        color: white;
      }
      tr:hover {
        background: #f1f1f1;
      }
      .footer {
        margin-top: 20px;
        text-align: center;
      }
      .footer a {
        text-decoration: none;
        color: #4CAF50;
        font-weight: bold;
      }
    </style>
    <a href="/ranking"> View Leaderboard</a>
  </head>
  <body>
    <h2>RFID Scan Log</h2>
    <table>
      <thead>
        <tr><th>UID</th><th>Timestamp</th></tr>
      </thead>
      <tbody id="logTable">
        <!-- rows will be inserted here -->
      </tbody>
    </table>
    <div class="footer">
      
      <button onclick="clearLog()"> Clear Log</button>
    </div>
    

    <script>
      async function fetchLog() {
        try {
          const res = await fetch("/data");
          const data = await res.json();
          const table = document.getElementById("logTable");
          table.innerHTML = "";
          data.forEach(row => {
            const tr = document.createElement("tr");
            tr.innerHTML = `<td>${row.uid}</td><td>${row.time}</td>`;
            table.appendChild(tr);
          });
        } catch (e) {
          console.error("Failed to fetch log:", e);
        }
      }

      setInterval(fetchLog, 3000); // every 3 seconds
      fetchLog(); // first call on page load

      // clear button
      async function clearLog() {
  if (!confirm("Are you sure you want to delete all scan logs?")) return;
  try {
    const res = await fetch("/clear", { method: "POST" });
    if (res.ok) {
      alert("Scan log cleared.");
      fetchLog(); // Refresh table
    } else {
      alert("Failed to clear log.");
    }
  } catch (e) {
    alert("Error clearing log.");
    console.error(e);
  }
}

    </script>
  </body>
  </html>
  )rawliteral";

  server.send(200, "text/html", page);
}

void handleDownload() {
  File file = LittleFS.open("/uids.txt", "r");
  if (!file) {
    server.send(404, "text/plain", "File not found");
    return;
  }
  server.streamFile(file, "text/plain");
  file.close();
}


void setup() {
 Serial.begin(115200);

  //static ip
  //if (!WiFi.config(local_IP, gateway, subnet)) {
  //  Serial.println(" Failed to configure static IP");
  //}

  WiFi.begin(ssid, password);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500); Serial.print(".");
  }
  Serial.println("\n Wi-Fi Connected!");
  Serial.print(" Visit: http://");
  Serial.println(WiFi.localIP());

  // LittleFS
  if (!LittleFS.begin()) {
    Serial.println(" LittleFS Mount Failed");
    return;
  }

  // time 
  configTime(TZ_Europe_Paris, "pool.ntp.org", "time.nist.gov");
  Serial.println(" Waiting for NTP time...");
  time_t now = time(nullptr);
  while (now < 100000) {
    delay(100);
    now = time(nullptr);
  }
  Serial.println(" Time synced!");


  // RFID

  mfrc522.PCD_Init();    // Init MFRC522 board.
  MFRC522Debug::PCD_DumpVersionToSerial(mfrc522, Serial); // Show details of PCD - MFRC522 Card Reader details.
  Serial.println(F("Scan PICC to see UID, SAK, type, and data blocks..."));

  // Web routes
  server.on("/", handleRoot);
  server.on("/download", handleDownload);

  server.begin();
  Serial.println(" Web server started");

server.on("/data", HTTP_GET, []() {
  if (!LittleFS.exists("/uids.txt")) {
    server.send(200, "application/json", "[]");
    return;
  }

  File file = LittleFS.open("/uids.txt", "r");
  String json = "[";
  bool first = true;

  while (file.available()) {
    String line = file.readStringUntil('\n');
    int sep = line.indexOf('|');
    if (sep > 0) {
      if (!first) json += ",";
      String uid = line.substring(0, sep - 1);
      String time = line.substring(sep + 2);
      json += "{\"uid\":\"" + uid + "\",\"time\":\"" + time + "\"}";
      first = false;
    }
  }
  file.close();
  json += "]";
  server.send(200, "application/json", json);
});
// Clean button

server.on("/clear", HTTP_POST, []() {
  LittleFS.remove("/uids.txt");
  server.send(200, "text/plain", "Log cleared");
});
//blinking initialize
pinMode(LED_BUILTIN, OUTPUT);     // Initialize the LED_BUILTIN pin as an output
digitalWrite(LED_BUILTIN, HIGH);  // Turn the LED off by making the voltage HIGH

//ranking page

server.on("/ranking", HTTP_GET, []() {
  if (!LittleFS.exists("/uids.txt")) {
    server.send(200, "text/html", "<h2>No scans yet.</h2>");
    return;
  }

  //ranking page
  File file = LittleFS.open("/uids.txt", "r");
  String page = "<!DOCTYPE html><html><head><meta charset='UTF-8'><title>RFID Leaderboard</title><style>";
  page += "body{font-family:Arial;background:#f0f0f0;padding:2em;}";
  page += "table{width:100%;border-collapse:collapse;background:white;box-shadow:0 0 5px rgba(0,0,0,0.1);}";
  page += "th,td{padding:12px;border-bottom:1px solid #ccc;text-align:left;}";
  page += "th{background:#2196F3;color:white;}";
  page += "</style></head><body><h2> RFID Leaderboard</h2><table><tr><th>Rank</th><th>Name</th><th>Timestamp</th></tr>";

  int rank = 1;
  while (file.available()) {
    String line = file.readStringUntil('\n');
    int sep = line.indexOf('|');
    if (sep > 0) {
      String name = line.substring(0, sep - 1);
      String time = line.substring(sep + 2);
      page += "<tr><td>" + String(rank++) + "</td><td>" + name + "</td><td>" + time + "</td></tr>";
    }
  }
  file.close();

  page += "</table></body></html>";
  server.send(200, "text/html", page);
});
//json
server.on("/json", HTTP_GET, []() {
  if (!LittleFS.exists("/uids.txt")) {
    server.send(200, "application/json", "[]");
    return;
  }

  File file = LittleFS.open("/uids.txt", "r");
  String json = "[";
  bool first = true;

  while (file.available()) {
    String line = file.readStringUntil('\n');
    int sep = line.indexOf('|');
    if (sep > 0) {
      if (!first) json += ",";
      String uid = line.substring(0, sep - 1);
      String time = line.substring(sep + 2);
      json += "{\"uid\":\"" + uid + "\",\"time\":\"" + time + "\"}";
      first = false;
    }
  }
  file.close();
  json += "]";
  server.send(200, "application/json", json);
});
}

void loop() {

  server.handleClient();

  // Reset the loop if no new card present on the sensor/reader. This saves the entire process when idle.
  if (!mfrc522.PICC_IsNewCardPresent()) {
    return;
  }

  // Select one of the cards.
  if (!mfrc522.PICC_ReadCardSerial()) {
    return;
  }

  // Dump debug info about the card; PICC_HaltA() is automatically called.
  MFRC522Debug::PICC_DumpToSerial(mfrc522, Serial, &(mfrc522.uid));
  
// Convert UID to string
  String uidString = "";
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    uidString += String(mfrc522.uid.uidByte[i], HEX);
    if (i < mfrc522.uid.size - 1) uidString += ":";
  }

  String name = uidToName.count(uidString) ? uidToName[uidString] : uidString;
  Serial.println("Scanned: " + name);


  // Check for duplicate before saving (optional)
  File fileCheck = LittleFS.open("/uids.txt", "r");
  bool alreadyExists = false;
  while (fileCheck.available()) {
    if (fileCheck.readStringUntil('\n') == uidString) {
      alreadyExists = true;
      break;
    }
  }
  fileCheck.close();

    if (!alreadyExists) {
    time_t now = time(nullptr);
    struct tm* timeinfo = localtime(&now);

    char timestamp[30];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", timeinfo);

    File file = LittleFS.open("/uids.txt", "a");
    if (file) {
      file.printf("%s | %s\n", name.c_str(), timestamp);
      file.close();
      Serial.printf(" UID saved with timestamp: %s\n", timestamp);
      digitalWrite(LED_BUILTIN, LOW);   // Turn the LED if the card is succesfully scanned
      delay(2000);                      // Wait for 2 seconds
      digitalWrite(LED_BUILTIN, HIGH);  // Turn the LED off 
    } else {
      Serial.println(" Failed to write UID");
    }
  }


  delay(2000);
}

