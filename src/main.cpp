#include <ESP8266WiFi.h>
#include <WiFiManager.h> // Подключение WiFiManager
#include <ESP8266mDNS.h> // Подключение mDNS
#include <ESP8266WebServer.h> // Подключение стандартного веб-сервера
#include <Adafruit_NeoPixel.h> // Для работы с адресной лентой

#define LED_PIN D4       // Пин подключения ленты
#define NUM_PIXELS 3     // Количество светодиодов в ленте

// Настройка адресной ленты
Adafruit_NeoPixel strip(NUM_PIXELS, LED_PIN, NEO_GRB + NEO_KHZ800);

// Веб-сервер
ESP8266WebServer server(80);

// Цветовая палитра
uint32_t colors[3] = {strip.Color(255, 0, 0), strip.Color(0, 255, 0), strip.Color(0, 0, 255)}; // Красный, зелёный, синий

// Функция для управления лентой
void setColor(uint32_t color) {
    for (int i = 0; i < NUM_PIXELS; i++) {
        strip.setPixelColor(i, color);
    }
    strip.show();
}

// Включение/выключение ленты
bool isOn = false;

void toggleLamp() {
    if (isOn) {
        setColor(0); // Выключить
        isOn = false;
    } else {
        setColor(colors[0]); // Включить с красным
        isOn = true;
    }
}

void setup() {
    Serial.begin(115200);

    // Настройка WiFi через WiFiManager
    WiFiManager wifiManager;
    wifiManager.autoConnect("MyLampSetup");

    // Успешное подключение к WiFi
    Serial.println("WiFi connected!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());

    // Настройка mDNS
    if (MDNS.begin("mylamp")) {
        Serial.println("mDNS responder started");
        Serial.println("Access your lamp at: http://mylamp.local");
    }

    // Инициализация ленты
    strip.begin();
    strip.show(); // Очистить ленту

    // Веб-страница управления
    server.on("/", HTTP_GET, []() {
        String html = R"rawliteral(
          <!DOCTYPE html>
          <html>
          <head>
            <title>My Lamp</title>
            <meta charset="UTF-8">
          </head>
          <body>
            <h1>Управление лампой</h1>
            <button onclick="fetch('/toggle')">Вкл/Выкл</button>
            <button onclick="fetch('/color?c=red')">Красный</button>
            <button onclick="fetch('/color?c=green')">Зелёный</button>
            <button onclick="fetch('/color?c=blue')">Синий</button>
          </body>
          </html>
        )rawliteral";
        server.send(200, "text/html", html);
    });

    // Обработчик включения/выключения
    server.on("/toggle", HTTP_GET, []() {
        toggleLamp();
        server.send(200, "text/plain", "Toggled!");
    });

    // Обработчик смены цвета
    server.on("/color", HTTP_GET, []() {
        if (server.hasArg("c")) {
            String color = server.arg("c");
            if (color == "red") {
                setColor(colors[0]);
            } else if (color == "green") {
                setColor(colors[1]);
            } else if (color == "blue") {
                setColor(colors[2]);
            }
            server.send(200, "text/plain", "Color changed!");
        } else {
            server.send(400, "text/plain", "Missing color parameter");
        }
    });

    // Запуск сервера
    server.begin();
}

void loop() {
    server.handleClient();
    MDNS.update();
}
