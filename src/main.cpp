#include <ESP8266WiFi.h>
#include <WiFiManager.h> 
#include <ESP8266WebServer.h>
#include <Adafruit_NeoPixel.h>
#include <ESP8266mDNS.h> // Подключение mDNS
#include <LittleFS.h> // Для работы с файловой системой

#define LED_PIN D4
#define NUM_PIXELS 3
#define DEFAULT_NAME "lamp"

Adafruit_NeoPixel strip(NUM_PIXELS, LED_PIN, NEO_GRB + NEO_KHZ800);
ESP8266WebServer server(80);

uint32_t colors[3] = {strip.Color(255, 0, 0), strip.Color(0, 255, 0), strip.Color(0, 0, 255)};
bool isOn = false;
char lampName[20] = DEFAULT_NAME; // Уникальное имя лампы (по умолчанию)

void setColor(uint32_t color) {
    for (int i = 0; i < NUM_PIXELS; i++) {
        strip.setPixelColor(i, color);
    }
    strip.show();
}

void turnOnLamp() {
    if (!isOn) {
        setColor(colors[0]); // Включаем лампу с первым цветом (красный по умолчанию)
        isOn = true;
    }
}

void turnOffLamp() {
    setColor(0); // Выключаем лампу
    isOn = false;
}

void setup() {
    Serial.begin(115200);

    if (!LittleFS.begin()) {
        Serial.println("An error occurred while mounting LittleFS");
        return;
    }

    // Загружаем сохранённое имя лампы, если есть
    if (LittleFS.exists("/lampName.txt")) {
        File file = LittleFS.open("/lampName.txt", "r");
        file.readBytesUntil('\n', lampName, sizeof(lampName));
        file.close();
    } else {
        strcpy(lampName, DEFAULT_NAME); // Если нет файла, используем имя по умолчанию
    }
    Serial.printf("Current lamp name: %s\n", lampName);

    WiFiManager wifiManager;

    // Добавляем поле для имени лампы
    WiFiManagerParameter customLampName("lampName", "Enter lamp name", lampName, 20);
    wifiManager.addParameter(&customLampName);

    wifiManager.autoConnect("LampSetup");

    Serial.println("WiFi connected!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());

     // Сохраняем новое имя лампы, если оно было изменено
    if (strcmp(lampName, customLampName.getValue()) != 0) {
        strcpy(lampName, customLampName.getValue());
        File file = LittleFS.open("/lampName.txt", "w");
        file.print(lampName);
        file.close();
        Serial.printf("New lamp name saved: %s\n", lampName);
    }

    // Настраиваем mDNS с именем лампы
    if (MDNS.begin(lampName)) {
        Serial.printf("mDNS responder started for %s.local\n", lampName);
    }

    strip.begin();
    strip.show();

    // Эндпоинт для отображения главной страницы
    server.on("/", HTTP_GET, []() {
        File file = LittleFS.open("/index.html", "r");
        if (!file) {
            server.send(500, "text/plain", "Failed to open file");
            return;
        }
        server.streamFile(file, "text/html");
        file.close();
    });

    // Эндпоинт для включения лампы
    server.on("/turnOn", HTTP_GET, []() {
        turnOnLamp();
        server.send(200, "text/plain", "Lamp turned on!");
    });

    // Эндпоинт для выключения лампы
    server.on("/turnOff", HTTP_GET, []() {
        turnOffLamp();
        server.send(200, "text/plain", "Lamp turned off!");
    });

    // Эндпоинт для смены цвета
    server.on("/color", HTTP_GET, []() {
        if (isOn) {
            if (server.hasArg("c")) {
                String color = server.arg("c");
                if (color.length() == 6) { // Ожидается формат RRGGBB
                    uint32_t r = strtol(color.substring(0, 2).c_str(), nullptr, 16);
                    uint32_t g = strtol(color.substring(2, 4).c_str(), nullptr, 16);
                    uint32_t b = strtol(color.substring(4, 6).c_str(), nullptr, 16);
                    setColor(strip.Color(r, g, b));
                    server.send(200, "text/plain", "Color changed!");
                } else {
                    server.send(400, "text/plain", "Invalid color format. Please use RRGGBB.");
                }
            } else {
                server.send(400, "text/plain", "Missing color parameter");
            }
        }
    });

    server.begin();
}

void loop() {
    server.handleClient();
    MDNS.update();
}
