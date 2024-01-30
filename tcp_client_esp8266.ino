#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <rdm6300.h>
#include <Ticker.h>

const char* ssid = "SSID";
const char* password = "Password";
const char* serverAddress = "tcp server";
const int serverPort = 7776;
#define RDM6300_RX_PIN 5
#define READ_LED_PIN 4
#define MAX_JSON_STRING_LENGTH 20

Rdm6300 rdm6300;
WiFiClient client;
ESP8266WebServer server(80);

// Variables to store the last three serial messages
String serialMessages[3];
int serialIndex = 0;

// Variable to store error messages
String errorMessages;

// Timer to clear error messages after 10 seconds
Ticker errorClearTimer;

// Function to connect to Wi-Fi
void connectToWiFi() {
  Serial.println("Connecting to Wi-Fi...");
  WiFi.begin(ssid, password);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nConnected to Wi-Fi");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
  } else {
    errorMessages = "Wi-Fi connection failed. Check your credentials or restart the device.";
    Serial.println("\n" + errorMessages);
  }
}

// Handler for the root URL
void handleRoot() {
  // Build the HTML response
  String html = "<html><body>";
  html += "<h1>Printer TCP client</h1>";
  html += "<p>Wi-Fi Status: " + String(WiFi.status() == WL_CONNECTED ? "Connected" : "Disconnected") + "</p>";
  html += "<p>IP Address: " + WiFi.localIP().toString() + "</p>";
  html += "<p><a href='/restart'>Reboot</a></p>";

  // Display the last three serial messages
  html += "<p>Rfid card messages:</p>";
  for (int i = 0; i < 1; i++) {
    html += "<p>" + serialMessages[(serialIndex + i) % 1] + "</p>";
  }

  // Add a separate field for errors
  html += "<h2>Error Log:</h2>";
  html += "<p>" + errorMessages + "</p>";
  html += "</body></html>";

  // Send the HTML response
  server.send(200, "text/html", html);
}

Ticker timer;

void restartESP() {
  Serial.println("Restarting...");
  ESP.restart();
}

// Feed the watchdog timer
void feedWatchdog() {
  ESP.wdtFeed();
}

// Handler for device restart
void handleRestart() {
  // Send the "Restarting..." response
  server.send(200, "text/plain", "Restarting...");

  // Set up a timer to restart the ESP after a longer delay (e.g., 5 seconds)
  timer.attach(5, restartESP);
}

void clearErrorMessages() {
  // Clear error messages
  errorMessages = "";
  Serial.println("Error messages cleared");
}

void setup() {
  Serial.begin(115200);

  connectToWiFi();

  pinMode(READ_LED_PIN, OUTPUT);
  digitalWrite(READ_LED_PIN, LOW);

  rdm6300.begin(RDM6300_RX_PIN);

  // Set up web server routes
  server.on("/", HTTP_GET, handleRoot);
  server.on("/restart", HTTP_GET, handleRestart);

  // Set up a timer to clear error messages after 10 seconds
  errorClearTimer.attach(10, clearErrorMessages);

  // Start the server
  server.begin();
}

void loop() {
  server.handleClient();
  feedWatchdog();
  handleRFIDCard();
  handleWiFiReconnection();
  // ... other logic
  delay(100);
}

void handleRFIDCard() {
  if (rdm6300.get_new_tag_id()) {
    uint32_t cardSerial = rdm6300.get_tag_id();
    char jsonString[MAX_JSON_STRING_LENGTH];
    snprintf(jsonString, MAX_JSON_STRING_LENGTH, "%X\r\n", cardSerial);
    Serial.println("Connecting to the server...");

    // Save the serial message to the array
    serialMessages[serialIndex] = jsonString;
    serialIndex = (serialIndex + 1) % 3;

    if (client.connect(serverAddress, serverPort)) {
      Serial.println("Connected to the server");
      client.println(jsonString);
      Serial.println("Data sent to the server");

      digitalWrite(READ_LED_PIN, HIGH);
      delay(500);
      digitalWrite(READ_LED_PIN, LOW);

      client.stop();
      Serial.println("Connection to the tcp server closed");
    } else {
      errorMessages = "Connection to the tcp server failed";
      Serial.println(errorMessages);
    }
  }
}

void handleWiFiReconnection() {
  if (WiFi.status() != WL_CONNECTED) {
    errorMessages = "Wi-Fi connection lost. Reconnecting...";
    Serial.println(errorMessages);
    connectToWiFi();
  }
}
