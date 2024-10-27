#include <WiFi.h>
#include <WebServer.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <MFRC522v2.h>
#include <MFRC522DriverSPI.h>
#include <MFRC522DriverPinSimple.h>
#include <SPI.h>
#include <ESPmDNS.h>
#include <map>

// Wi-Fi credentials
const char* ssid = "chittethe";
const char* password = "vishnu@123";

// Google Apps Script URL
const char* googleScriptURL = "https://script.google.com/macros/s/AKfycbwi0PnBHfj1_3uIKoD_yxIAYBbTPytMWGlVIew7xRFwURSRae2BVTDjjvCftNL7M3fhqA/exec";

// RFID settings
#define SS_PIN 22    // RFID SS pin (SDA)
#define RST_PIN 21   // RFID RST pin

// LED indicator pins
#define LED_READY_PIN 5    // GPIO pin for "Ready to Scan" LED
#define LED_MARK_PIN 4     // GPIO pin for "Marking Attendance" LED

// Create an MFRC522DriverPinSimple object for SS_PIN
MFRC522DriverPinSimple ss_pin(SS_PIN);

// Create the SPI driver with the ss_pin
MFRC522DriverSPI driver{ss_pin};

// Create the MFRC522 instance with the driver
MFRC522 rfid{driver};

WebServer server(80);
String scannedRFID = "";  // Holds the last scanned RFID tag ID
String lastScannedTag = "";
String lastScanStatus = "";

// Attendance session variables
bool attendanceSessionActive = false;
std::map<String, unsigned long> lastScanTime;
const unsigned long SCAN_INTERVAL = 5000; // 5 seconds debounce time

// Common CSS styles
const char* commonCSS = "\
body { font-family: Arial, sans-serif; margin: 0; padding: 0; background-color: #e9ecef; }\
.container { display: flex; flex-direction: column; align-items: center; justify-content: center; min-height: 100vh; text-align: center; }\
h1, h2, h3 { color: #343a40; margin-bottom: 20px; }\
p { font-size: 18px; color: #495057; margin-bottom: 20px; }\
a { text-decoration: none; }\
ul { list-style-type: none; padding: 0; }\
li { background-color: #fff; margin: 5px; padding: 15px; border-radius: 5px; width: 80%; max-width: 400px; box-shadow: 0 2px 4px rgba(0,0,0,0.1); }\
button { padding: 10px 20px; font-size: 16px; margin: 10px; cursor: pointer; border: none; border-radius: 5px; }\
.btn-start { background-color: #28a745; color: #fff; }\
.btn-stop { background-color: #dc3545; color: #fff; }\
.btn-reset { background-color: #ffc107; color: #212529; }\
.btn-scan { background-color: #17a2b8; color: #fff; }\
.btn-primary { background-color: #007BFF; color: #fff; }\
button:hover { opacity: 0.9; }";

void setup() {
  Serial.begin(115200);
  Serial.println(F("Starting setup..."));

  // Initialize SPI bus
  SPI.begin(); // Initialize SPI bus with default pins
  Serial.println(F("SPI bus initialized."));

  // Initialize the reset pin
  pinMode(RST_PIN, OUTPUT);
  digitalWrite(RST_PIN, HIGH);  // Set RST_PIN to HIGH
  Serial.println(F("RFID reset pin initialized."));

  // Initialize the RFID reader
  rfid.PCD_Init(); // Init MFRC522
  Serial.println(F("RFID reader initialized."));

  // Initialize LED pins
  pinMode(LED_READY_PIN, OUTPUT);
  digitalWrite(LED_READY_PIN, LOW);  // Initially off

  pinMode(LED_MARK_PIN, OUTPUT);
  digitalWrite(LED_MARK_PIN, LOW);   // Initially off

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  Serial.print(F("Connecting to WiFi..."));
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(F("."));
  }
  Serial.println(F("\nConnected to WiFi"));

  // Print the ESP32 IP address
  Serial.print(F("ESP32 IP Address: "));
  Serial.println(WiFi.localIP());

  // Initialize mDNS
  if (MDNS.begin("log")) {
    Serial.println(F("mDNS responder started at http://log.local"));
  } else {
    Serial.println(F("Error starting mDNS"));
  }

  // Setup web server routes
  server.on("/", handleRoot);
  server.on("/scan", handleScan);
  server.on("/register", HTTP_GET, handleRegister);
  server.on("/register", HTTP_POST, handleRegister);
  server.on("/remove", handleRemove);
  server.on("/start", handleStartAttendance);
  server.on("/reset", handleResetAttendance);
  server.on("/stop", handleStopAttendance);
  server.on("/status", handleStatus); // Added route for status page

  server.begin();
  Serial.println(F("Web server started"));
}

void loop() {
  server.handleClient();

  if (attendanceSessionActive) {
    handleRFIDScanning();
  }
}

// Handler for the root page
void handleRoot() {
  Serial.println(F("Handling root page..."));
  String html;
  html.reserve(2048); // Adjust size as needed

  html = F("<!DOCTYPE html><html><head><title>Bus Attendance</title>");
  html += F("<meta name='viewport' content='width=device-width, initial-scale=1.0'>");

  // Include CSS styles
  html += F("<style>");
  html += commonCSS;
  html += F("</style></head><body>");

  // Wrap content in a container div
  html += F("<div class='container'>");
  html += F("<h1>Bus Attendance System</h1>");

  // Fetch the attendance list from Google Sheets
  if (WiFi.status() == WL_CONNECTED) {

    WiFiClientSecure client;
    client.setInsecure();

    HTTPClient https;
    String url = String(googleScriptURL) + "?action=getList";
    https.begin(client, url);
    https.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);

    int httpCode = https.GET();
    String payload = https.getString(); // Get the response payload
    Serial.println("HTTP GET code: " + String(httpCode));
    Serial.println("Response payload: " + payload);

    if (httpCode == 200 || httpCode == 302) {
      DynamicJsonDocument doc(8192); // Adjust size as needed
      DeserializationError error = deserializeJson(doc, payload);
      if (error) {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.f_str());
        Serial.println("Payload was: " + payload); // Print payload for debugging
        html += F("<p>Error parsing data from Google Sheets.</p>");
      } else {
        if (!doc.containsKey("totalStudents") || !doc.containsKey("studentsOnBus") || !doc.containsKey("students")) {
          Serial.println(F("JSON data missing expected keys."));
          html += F("<p>Error: Unexpected data format received.</p>");
        } else {
          int totalStudents = doc["totalStudents"];
          int studentsOnBus = doc["studentsOnBus"];

          html += "<p>Total Students: " + String(totalStudents) + "</p>";
          html += "<p>Students on Bus: " + String(studentsOnBus) + "</p>";

          html += F("<h3>Students Currently on the Bus:</h3>");
          if (studentsOnBus > 0) {
            html += F("<ul>");
            for (JsonObject student : doc["students"].as<JsonArray>()) {
              if (student["onBus"]) {
                String name = String(student["name"].as<const char*>());
                String time = String(student["time"].as<const char*>());
                html += "<li>" + name + " - " + time + "</li>";
              }
            }
            html += F("</ul>");
          } else {
            html += F("<p>No students are currently on the bus.</p>");
          }
        }
      }
    } else {
      html += "<p>Error fetching data from Google Sheets. HTTP Code: " + String(httpCode) + "</p>";
      Serial.println("Error fetching data. HTTP Code: " + String(httpCode));
    }
    https.end();
  } else {
    html += F("<p>Wi-Fi not connected.</p>");
    Serial.println(F("Wi-Fi not connected."));
  }

  // Links to other pages with styled buttons
  html += F("<p><a href='/start'><button class='btn-start'>Start Attendance Session</button></a></p>");
  html += F("<p><a href='/reset'><button class='btn-reset'>Reset Attendance Session</button></a></p>");
  html += F("<p><a href='/scan'><button class='btn-scan'>Register/Remove Student</button></a></p>");

  html += F("</div></body></html>");

  server.send(200, "text/html", html);
}

// Handler for scanning RFID tags (single scan)
void handleScan() {
  Serial.println(F("Attempting to scan for RFID tag..."));
  String html;
  html.reserve(1024); // Adjust size as needed

  html = F("<!DOCTYPE html><html><head><title>Scan RFID</title>");
  html += F("<meta name='viewport' content='width=device-width, initial-scale=1.0'>");
  html += F("<style>");
  html += commonCSS;
  html += F("</style></head><body>");
  html += F("<div class='container'>");
  html += F("<h2>Scan an RFID tag</h2>");

  // Look for new cards
  if (!rfid.PICC_IsNewCardPresent()) {
    Serial.println(F("No new card present."));
    html += F("<p>No RFID tag detected. Please try again.</p>");
    html += F("<p><a href='/scan'><button class='btn-primary'>Retry</button></a> <a href='/'><button class='btn-primary'>Back to Home</button></a></p>");
  } else if (!rfid.PICC_ReadCardSerial()) {
    Serial.println(F("Error reading card serial."));
    html += F("<p>Error reading RFID tag. Please try again.</p>");
    html += F("<p><a href='/scan'><button class='btn-primary'>Retry</button></a> <a href='/'><button class='btn-primary'>Back to Home</button></a></p>");
  } else {
    // Read RFID UID and store it in `scannedRFID`
    scannedRFID = "";
    for (byte i = 0; i < rfid.uid.size; i++) {
      scannedRFID += String(rfid.uid.uidByte[i], HEX);
    }
    rfid.PICC_HaltA(); // Halt PICC

    Serial.println("RFID tag detected: " + scannedRFID);

    html += "<p>RFID Scanned: " + scannedRFID + "</p>";
    html += F("<p><a href='/register'><button class='btn-primary'>Register Student</button></a> <a href='/remove'><button class='btn-primary'>Remove Student</button></a></p>");
    html += F("<p><a href='/'><button class='btn-primary'>Back to Home</button></a></p>");
  }

  html += F("</div></body></html>");
  server.send(200, "text/html", html);
}

// Function to mark attendance in Google Sheets
void markAttendance(String rfidTag) {
  lastScannedTag = rfidTag; // Update last scanned tag

  // Turn on "Marking Attendance" LED
  digitalWrite(LED_MARK_PIN, HIGH);

  if (WiFi.status() == WL_CONNECTED) {
    WiFiClientSecure client;
    client.setInsecure();

    HTTPClient https;
    String url = String(googleScriptURL) + "?action=markAttendance&rfid=" + rfidTag;
    https.begin(client, url);
    https.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);

    int httpCode = https.GET();
    String payload = https.getString();
    Serial.println("HTTP GET code (markAttendance): " + String(httpCode));
    Serial.println("Response payload: " + payload);

    if (httpCode == 200 || httpCode == 302) {
      if (payload.indexOf("Attendance updated.") >= 0) {
        lastScanStatus = "Attendance marked successfully.";
        Serial.println("Attendance marked for RFID: " + rfidTag);
      } else if (payload.indexOf("RFID not found.") >= 0) {
        lastScanStatus = "RFID not found in the system.";
        Serial.println("RFID not found: " + rfidTag);
      } else {
        lastScanStatus = "Failed to mark attendance.";
        Serial.println("Failed to mark attendance. Payload: " + payload);
      }
    } else {
      lastScanStatus = "Error communicating with server.";
      Serial.println("Failed to mark attendance. HTTP Code: " + String(httpCode));
    }
    https.end();
  } else {
    lastScanStatus = "Wi-Fi not connected.";
    Serial.println(F("Wi-Fi not connected."));
  }

  // Turn off "Marking Attendance" LED
  digitalWrite(LED_MARK_PIN, LOW);

  // Turn on "Ready to Scan" LED
  digitalWrite(LED_READY_PIN, HIGH);

  Serial.println("Last Scanned Tag: " + lastScannedTag);
  Serial.println("Last Scan Status: " + lastScanStatus);
}

// Handler for registering a new student
void handleRegister() {
  Serial.println(F("Handling register page..."));
  if (scannedRFID == "") {
    server.send(200, "text/html", F("<h2>No RFID scanned. Go to <a href='/scan'>Scan RFID</a> to scan a tag first.</h2>"));
    return;
  }

  if (server.method() == HTTP_GET) {
    // Display the registration form
    String html;
    html.reserve(1024);
    html = F("<!DOCTYPE html><html><head><title>Register Student</title>");
    html += F("<meta name='viewport' content='width=device-width, initial-scale=1.0'>");
    html += F("<style>");
    html += commonCSS;
    html += F("input { padding: 10px; margin: 5px; font-size: 16px; width: 80%; max-width: 300px; }");
    html += F("</style></head><body>");
    html += F("<div class='container'>");
    html += "<h2>Register Student with RFID: " + scannedRFID + "</h2>";
    html += F("<form action='/register' method='post'>");
    html += F("Name:<br><input type='text' name='name'><br><br>");
    html += F("Admission Number:<br><input type='text' name='admission_number'><br><br>");
    html += F("<input type='submit' value='Register' class='btn-primary'>");
    html += F("</form>");
    html += F("<p><a href='/'><button class='btn-primary'>Back to Home</button></a></p>");
    html += F("</div></body></html>");
    server.send(200, "text/html", html);
  } else if (server.method() == HTTP_POST) {
    // Handle form submission
    String name = server.arg("name");
    String admissionNumber = server.arg("admission_number");

    // Validate input
    if (name.length() == 0 || admissionNumber.length() == 0) {
      server.send(200, "text/html", F("<h2>Please enter both name and admission number.</h2><p><a href='/register'><button class='btn-primary'>Go back</button></a></p>"));
      return;
    }

    // URL encode the name and admission number
    String nameEncoded = URLEncode(name.c_str());
    String admissionEncoded = URLEncode(admissionNumber.c_str());

    // Send registration details to Google Sheets
    if (WiFi.status() == WL_CONNECTED) {
      WiFiClientSecure client;
      client.setInsecure();

      HTTPClient https;
      String url = String(googleScriptURL) + "?action=register&rfid=" + scannedRFID + "&name=" + nameEncoded + "&admission=" + admissionEncoded;
      https.begin(client, url);
      https.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);

      int httpCode = https.GET();
      String payload = https.getString(); // Get response body
      Serial.println("HTTP GET code (register): " + String(httpCode));
      Serial.println("Response payload: " + payload);

      String html;
      html.reserve(1024);
      html = F("<div class='container'>");
      if (payload.indexOf("Student registered.") >= 0) {
        html += F("<h2>Student registered successfully!</h2><p><a href='/scan'><button class='btn-primary'>Register Another Student</button></a></p><p><a href='/'><button class='btn-primary'>Back to Home</button></a></p>");
      } else if (payload.indexOf("RFID already registered.") >= 0) {
        html += F("<h2>RFID already registered.</h2><p><a href='/scan'><button class='btn-primary'>Register Another Student</button></a></p><p><a href='/'><button class='btn-primary'>Back to Home</button></a></p>");
      } else {
        html += "<h2>Failed to register student.</h2><p>Error: " + payload + "</p><p><a href='/register'><button class='btn-primary'>Try again</button></a></p>";
      }
      html += F("</div>");
      server.send(200, "text/html", html);
      https.end();
    } else {
      server.send(200, "text/html", F("<h2>Wi-Fi not connected.</h2>"));
    }
    scannedRFID = "";  // Clear RFID after registration
  }
}

// Handler for removing a student
void handleRemove() {
  Serial.println(F("Handling remove page..."));
  if (scannedRFID == "") {
    server.send(200, "text/html", F("<h2>No RFID scanned. Go to <a href='/scan'>Scan RFID</a> to scan a tag first.</h2>"));
    return;
  }

  // Confirm removal
  if (WiFi.status() == WL_CONNECTED) {
    WiFiClientSecure client;
    client.setInsecure();

    HTTPClient https;
    String url = String(googleScriptURL) + "?action=remove&rfid=" + scannedRFID;
    https.begin(client, url);
    https.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);

    int httpCode = https.GET();
    String payload = https.getString(); // Get response body
    Serial.println("HTTP GET code (remove): " + String(httpCode));
    Serial.println("Response payload: " + payload);

    String html;
    html.reserve(1024);
    html = F("<div class='container'>");
    if (payload.indexOf("Student removed.") >= 0) {
      html += F("<h2>Student removed successfully!</h2><p><a href='/scan'><button class='btn-primary'>Remove Another Student</button></a></p><p><a href='/'><button class='btn-primary'>Back to Home</button></a></p>");
    } else if (payload.indexOf("RFID not found.") >= 0) {
      html += F("<h2>RFID not found.</h2><p><a href='/scan'><button class='btn-primary'>Remove Another Student</button></a></p><p><a href='/'><button class='btn-primary'>Back to Home</button></a></p>");
    } else {
      html += "<h2>Failed to remove student.</h2><p>Error: " + payload + "</p><p><a href='/'><button class='btn-primary'>Back to Home</button></a></p>";
    }
    html += F("</div>");
    server.send(200, "text/html", html);
    https.end();
  } else {
    server.send(200, "text/html", F("<h2>Wi-Fi not connected.</h2>"));
  }
  scannedRFID = "";  // Clear RFID after removal
}

// Function to URL encode strings
String URLEncode(const char* msg) {
  const char *hex = "0123456789ABCDEF";
  String encodedMsg = "";
  while (*msg != '\0') {
    if (('a' <= *msg && *msg <= 'z') ||
        ('A' <= *msg && *msg <= 'Z') ||
        ('0' <= *msg && *msg <= '9') ||
        (*msg == '-') || (*msg == '_') || (*msg == '.') || (*msg == '~')) {
      encodedMsg += *msg;
    } else {
      encodedMsg += '%';
      encodedMsg += hex[(*msg >> 4) & 0x0F];
      encodedMsg += hex[*msg & 0x0F];
    }
    msg++;
  }
  return encodedMsg;
}

// Handler to start the attendance session
void handleStartAttendance() {
  Serial.println(F("Starting attendance session..."));

  attendanceSessionActive = true;

  // Turn on "Ready to Scan" LED
  digitalWrite(LED_READY_PIN, HIGH);


  String html;
  html.reserve(1024); // Adjust size as needed
  html = F("<!DOCTYPE html><html><head><title>Start Attendance</title>");
  html += F("<meta name='viewport' content='width=device-width, initial-scale=1.0'>");
  html += F("<style>");
  html += commonCSS;
  html += F("</style></head><body>");
  html += F("<div class='container'>");
  html += F("<h2>Attendance Session Started</h2>");
  html += F("<p>Please scan your RFID tags.</p>");

  if (lastScannedTag != "") {
    html += "<p>Last Scanned RFID: " + lastScannedTag + "</p>";
    html += "<p>Status: " + lastScanStatus + "</p>";
  }

  html += F("<p><a href='/status'><button class='btn-primary'>View Attendance Status</button></a></p>");
  html += F("<p><a href='/stop'><button class='btn-stop'>Stop Attendance Session</button></a></p>");
  html += F("<p><a href='/reset'><button class='btn-reset'>Reset Attendance Session</button></a></p>");
  html += F("</div></body></html>");
  server.send(200, "text/html", html);
}

// Handler to stop the attendance session
void handleStopAttendance() {
  Serial.println(F("Stopping attendance session..."));

  attendanceSessionActive = false;

  // Turn off both LEDs
  digitalWrite(LED_READY_PIN, LOW);
  digitalWrite(LED_MARK_PIN, LOW);

  String html;
  html.reserve(1024); // Adjust size as needed
  html = F("<!DOCTYPE html><html><head><title>Attendance Stopped</title>");
  html += F("<meta name='viewport' content='width=device-width, initial-scale=1.0'>");
  html += F("<style>");
  html += commonCSS;
  html += F("</style></head><body>");
  html += F("<div class='container'>");
  html += F("<h2>Attendance Session Stopped</h2>");
  html += F("<p><a href='/'><button class='btn-primary'>Back to Home</button></a></p>");
  html += F("</div></body></html>");
  server.send(200, "text/html", html);
}

// Handler to reset the attendance session
void handleResetAttendance() {
  Serial.println(F("Resetting attendance session..."));

  // Send reset command to Google Apps Script
  if (WiFi.status() == WL_CONNECTED) {
    WiFiClientSecure client;
    client.setInsecure();

    HTTPClient https;
    String url = String(googleScriptURL) + "?action=resetAttendance";
    https.begin(client, url);
    https.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);

    int httpCode = https.GET();
    String payload = https.getString(); // Get response body
    Serial.println("HTTP GET code (resetAttendance): " + String(httpCode));
    Serial.println("Response payload: " + payload);

    String html;
    html.reserve(1024);
    html = F("<div class='container'>");
    if (payload.indexOf("Attendance reset.") >= 0) {
      html += F("<h2>Attendance session reset successfully!</h2><p><a href='/'><button class='btn-primary'>Back to Home</button></a></p>");
    } else {
      html += "<h2>Failed to reset attendance session.</h2><p>Error: " + payload + "</p><p><a href='/'><button class='btn-primary'>Back to Home</button></a></p>";
    }
    html += F("</div>");
    server.send(200, "text/html", html);
    https.end();
  } else {
    server.send(200, "text/html", F("<h2>Wi-Fi not connected.</h2>"));
  }
}

// Function to handle RFID scanning during attendance session
void handleRFIDScanning() {
  // Look for new RFID tags
  if (!rfid.PICC_IsNewCardPresent()) {
    return;
  }
  if (!rfid.PICC_ReadCardSerial()) {
    return;
  }

  // Turn off "Ready to Scan" LED
  digitalWrite(LED_READY_PIN, LOW);

  // Read RFID UID
  String rfidTag = "";
  for (byte i = 0; i < rfid.uid.size; i++) {
    rfidTag += String(rfid.uid.uidByte[i], HEX);
  }
  rfid.PICC_HaltA(); // Halt PICC
  Serial.println("RFID tag scanned during attendance session: " + rfidTag);

  unsigned long currentTime = millis();
  if (lastScanTime.find(rfidTag) == lastScanTime.end() || (currentTime - lastScanTime[rfidTag] > SCAN_INTERVAL)) {
    // Mark attendance
    markAttendance(rfidTag);
    lastScanTime[rfidTag] = currentTime;
    Serial.println("Attendance marked for tag: " + rfidTag);
  } else {
    Serial.println("Tag " + rfidTag + " scanned too recently. Ignoring.");

    // Turn on "Ready to Scan" LED
    digitalWrite(LED_READY_PIN, HIGH);
  }
}

// Handler for the status page
void handleStatus() {
  Serial.println(F("Handling status page..."));

  String html;
  html.reserve(1024); // Adjust size as needed
  html = F("<!DOCTYPE html><html><head><title>Scan Status</title>");
  html += F("<meta http-equiv='refresh' content='2'>"); // Auto-refresh every 2 seconds
  html += F("<meta name='viewport' content='width=device-width, initial-scale=1.0'>");
  html += F("<style>");
  html += commonCSS;
  html += F("</style></head><body>");
  html += F("<div class='container'>");
  html += F("<h2>Attendance Status</h2>");

  if (lastScannedTag != "") {
    html += "<p>Last Scanned RFID: " + lastScannedTag + "</p>";
    html += "<p>Status: " + lastScanStatus + "</p>";
  } else {
    html += F("<p>No scans yet.</p>");
  }

  html += F("<p><a href='/'><button class='btn-primary'>Back to Home</button></a></p>");
  html += F("</div></body></html>");
  server.send(200, "text/html", html);
}
