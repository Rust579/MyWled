#include <ESP8266WiFi.h>
#include <WiFiManager.h> 
#include <ESP8266WebServer.h>
#include <Adafruit_NeoPixel.h>
#include <ESP8266mDNS.h> // Подключение mDNS
#include <LittleFS.h> // Для работы с файловой системой

#define LED_PIN D4
#define NUM_PIXELS 3

Adafruit_NeoPixel strip(NUM_PIXELS, LED_PIN, NEO_GRB + NEO_KHZ800);
ESP8266WebServer server(80);

uint32_t colors[3] = {strip.Color(255, 0, 0), strip.Color(0, 255, 0), strip.Color(0, 0, 255)};
bool isOn = false;

void setColor(uint32_t color) {
    for (int i = 0; i < NUM_PIXELS; i++) {
        strip.setPixelColor(i, color);
    }
    strip.show();
}

void toggleLamp() {
    if (isOn) {
        setColor(0);
        isOn = false;
    } else {
        setColor(colors[0]);
        isOn = true;
    }
}

void setup() {
    Serial.begin(115200);

    WiFiManager wifiManager;
    wifiManager.autoConnect("MyLampSetup");

    Serial.println("WiFi connected!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());

    // Настройка mDNS
    if (MDNS.begin("mylamp")) {
        Serial.println("mDNS responder started");
        Serial.println("Access your lamp at: http://mylamp.local");
    }

    if (!LittleFS.begin()) {
        Serial.println("An error occurred while mounting LittleFS");
        return;
    }

    strip.begin();
    strip.show();

    server.on("/", HTTP_GET, []() {
        File file = LittleFS.open("/index.html", "r");
        if (!file) {
            server.send(500, "text/plain", "Failed to open file");
            return;
        }
        server.streamFile(file, "text/html");
        file.close();
    });

    server.on("/toggle", HTTP_GET, []() {
        toggleLamp();
        server.send(200, "text/plain", "Toggled!");
    });

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

    server.begin();
}

void loop() {
    server.handleClient();
    MDNS.update();
}
