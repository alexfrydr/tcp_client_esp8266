#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <rdm6300.h>
// Настройки для подключения к Wi-Fi
const char* ssid = "Ssid";
const char* password = "pass";
// Настройки для TCP сервера
const char* serverAddress = "tcp-server"; // IP адрес сервера
const int serverPort = 7776; // Порт сервера
#define RDM6300_RX_PIN 5 // Порт RDM6300
#define READ_LED_PIN 4 // Пин подключения диода
// Объект для работы с RDM6300
Rdm6300 rdm6300;
// Объект для работы с TCP клиентом
WiFiClient client;
void setup() {
  Serial.begin(115200);
  // Подключение к Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
  pinMode(READ_LED_PIN, OUTPUT);
  digitalWrite(READ_LED_PIN, LOW);
  // Настройка RDM6300
  rdm6300.begin(RDM6300_RX_PIN);
}
void loop() {
  // Чтение данных с RDM6300
  if (rdm6300.get_new_tag_id()) {
    uint32_t cardSerial = rdm6300.get_tag_id();
    // Конструкция строки JSON
    String jsonString = String(cardSerial, HEX) + "\r\n";
    Serial.println("Connecting to server...");
    // Отправка данных на сервер
    if (client.connect(serverAddress, serverPort)) {
      Serial.println("Connected to server");
      // Отправка данных на сервер
      client.println(jsonString);
      Serial.println("Data sent to server");
      // Моргнуть диодом после отправки
      digitalWrite(READ_LED_PIN, HIGH);
      delay(500); 
      digitalWrite(READ_LED_PIN, LOW);
      // Отключиться от сервера
      client.stop();
      Serial.println("Connection to server closed");
    } else {
      Serial.println("Connection to server failed");
    }
  }
  delay(100); // Задержка повтора цикла
}
