#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <rdm6300.h>
#include <Ticker.h>

const char* ssid = "SSid";
const char* password = "password";
const char* serverAddress = "tcp_server";
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
    Serial.println("\nWi-Fi connection failed. Check your credentials or restart the device.");
  }
}

// Handler for the root URL
void handleRoot() {
  // Build the HTML response
  String html = "<html><body>";
  html += "<h1>TCP client</h1>";
  html += "<p>Wi-Fi Status: " + String(WiFi.status() == WL_CONNECTED ? "Connected" : "Disconnected") + "</p>";
  html += "<p>IP Address: " + WiFi.localIP().toString() + "</p>";
  html += "<p><a href='/restart'>Reboot</a></p>";

  // Display the last three serial messages
  html += "<p>Last three serial messages:</p>";
  for (int i = 0; i < 3; i++) {
    html += "<p>" + serialMessages[(serialIndex + i) % 3] + "</p>";
  }

  html += "</body></html>";

  // Send the HTML response
  server.send(200, "text/html", html);
}

Ticker timer;

void restartESP() {
  Serial.println("Restarting...");
  ESP.restart();
}

// Handler for device restart
void handleRestart() {
  // Send the "Restarting..." response
  server.send(200, "text/plain", "Restarting...");

  // Set up a timer to restart the ESP in a moment
  timer.attach(1, restartESP);
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

  // Start the server
  server.begin();
}

void loop() {
  server.handleClient();

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
      Serial.println("Connection to the server closed");
    } else {
      Serial.println("Connection to the server failed");
    }
  }

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Wi-Fi connection lost. Reconnecting...");
    connectToWiFi();
  }

  // Other loop logic can be added here

  delay(100);
}
