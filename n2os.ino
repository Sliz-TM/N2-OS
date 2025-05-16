#include <ESP8266WiFi.h>
#include <Wire.h>
#include <RTClib.h>
#include <DHT.h>
#include <ESP8266WebServer.h>
#include <SSD1306.h>  // Используем эту библиотеку для дисплея

#define DHTPIN D3       // Pin for DHT11 sensor
#define DHTTYPE DHT11   // DHT11 sensor type

#define BUZZER_PIN D5   // Pin for the buzzer
#define SCREEN_WIDTH 128 // OLED display width
#define SCREEN_HEIGHT 64 // OLED display height

SSD1306 display(0x3C, 14, 12);  // Инициализация дисплея с адресом и пинами (поменял на D1 и D2)

DHT dht(DHTPIN, DHTTYPE);  // Initialize DHT
RTC_DS3231 rtc;            // Initialize DS3231 RTC
ESP8266WebServer server(80); // Web server running on port 80

const char* ssid = "Denis";         // Wi-Fi SSID
const char* password = "WiSher1983"; // Wi-Fi password

String terminal_output = "%NaN V2%>>>";  // Terminal prompt
String terminal_input = "";               // User input for commands

// Инициализация дисплея и других компонентов
bool animationDone = false;  // Флаг анимации

void setup() {
  Serial.begin(115200);

  // Initialize the display
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {  // Используем begin вместо init для правильной инициализации
    Serial.println(F("SSD1306 allocation failed"));
    for (;;);
  }
  display.display();
  delay(2000);  // Pause for 2 seconds

  // Setup the display
  display.clear();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);

  // Initialize DHT
  dht.begin();

  // Initialize RTC
  if (!rtc.begin()) {
    display.println("RTC not found");
    display.display();
    while (1);
  }

  // Connect to Wi-Fi
  display.clear();
  display.print("Scanning Wi-Fi...");
  display.display();
  
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    display.print(".");
    display.display();
  }

  // Display IP address after connection
  display.clear();
  display.print("Connected!");
  display.print("\nIP: ");
  display.println(WiFi.localIP());
  display.display();

  // Create web server
  server.on("/", HTTP_GET, handleRoot);
  server.on("/settime", HTTP_POST, handleSetTime);
  server.on("/status", HTTP_GET, handleStatus);
  server.on("/help", HTTP_GET, handleHelp);
  server.on("/os", HTTP_GET, handleOS);
  server.on("/unfi", HTTP_GET, handleUNFI);
  server.on("/temp", HTTP_GET, handleTemp);

  server.begin();
}

void loop() {
  server.handleClient();  // Handle HTTP requests
  
  if (!animationDone) {
    // Анимация перед запуском ОС
    for (int x = 128; x >= 64; x -= 2) {
      display.clear();
      drawWheel(x, 32, 0); // Рисуем колесо в текущей позиции
      display.display();
      delay(30); // Задержка между кадрами
    }

    // Остановка колеса в центре и его вращение
    for (int i = 0; i < 150; i++) { // Вращение в течение 5 секунд
      display.clear();
      drawWheel(64, 32, i * 10); // Угол вращения увеличивается
      display.display();
      delay(30);
    }

    // Уезд колеса влево
    for (int x = 64; x >= -40; x -= 2) {
      display.clear();
      drawWheel(x, 32, 0); // Рисуем колесо в текущей позиции
      display.display();
      delay(30);
    }

    // После выполнения анимации установить флаг
    animationDone = true;
  }

  // Отображение текста "N2 OS" постоянно
  displayText();
}

// Функция для рисования колеса
void drawWheel(int x, int y, int angle) {
  display.setColor(SSD1306_WHITE); // Белый цвет для колеса
  display.drawCircle(x, y, 20);  // Внешний круг (колесо)
  display.drawCircle(x, y, 15);  // Внутренний круг (детали)
  
  // Линии для вращения внутри колеса
  float rad = angle * 3.14159 / 180.0; // Преобразование в радианы
  display.drawLine(x + cos(rad) * 10, y + sin(rad) * 10, 
                   x - cos(rad) * 10, y - sin(rad) * 10); // Линия механизма
  display.drawLine(x + cos(rad + 3.14159 / 2) * 10, y + sin(rad + 3.14159 / 2) * 10,
                   x - cos(rad + 3.14159 / 2) * 10, y - sin(rad + 3.14159 / 2) * 10); // Вторая линия механизма

  // Добавление знака бесконечности внутри колеса
  display.setColor(SSD1306_BLACK);    // Черный для надписей внутри
  display.drawString(x - 5, y - 10, "∞");

  // Добавление знаков "+" и "-" в окошечках колеса
  display.drawString(x - 10, y - 5, "+");
  display.drawString(x + 5, y + 5, "-");
}

// Функция для отображения текста "N2 OS"
void displayText() {
  display.clear();
  display.setFont(ArialMT_Plain_24); // Настройка шрифта
  display.setColor(SSD1306_WHITE); // Белый цвет для текста
  int textWidth = display.getStringWidth("N2 OS"); // Вычисляем ширину текста
  int textHeight = 24; // Высота текста с учетом выбранного шрифта

  // Отображаем текст по центру
  display.drawString((128 - textWidth) / 2, (64 - textHeight) / 2, "N2 OS"); // Центрируем текст по горизонтали и вертикали
  display.display();
}

void handleRoot() {
  String html = "<html><head><title>N2 OS Desktop</title></head><body>";
  html += "<h1>Loading N2 OS...</h1>";
  html += "<div id='loading'>Loading...</div>";
  html += "<script>setTimeout(function(){document.getElementById('loading').innerHTML = 'N2 OS';}, 3000);</script>";
  html += "</body></html>";
  server.send(200, "text/html", html);
}

void handleSetTime() {
  String time = server.arg("time"); // Get time input from form
  if (rtc.isrunning()) {
    int hours = time.substring(0, 2).toInt();
    int minutes = time.substring(3, 5).toInt();
    int seconds = time.substring(6, 8).toInt();
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__))); // Apply the entered time
    display.clear();
    display.print("Time set to: ");
    display.print(hours);
    display.print(":");
    display.print(minutes);
    display.print(":");
    display.println(seconds);
    display.display();
    server.send(200, "text/plain", "Time updated.");
    playTone();
  }
}

void handleStatus() {
  String status = "System Information:\n";
  status += "WiFi: " + String(WiFi.localIP()) + "\n";
  status += "DS3231: Time synced\n";
  status += "DHT11: Temperature: " + String(dht.readTemperature()) + "C\n";
  server.send(200, "text/plain", status);
}

void handleHelp() {
  String helpText = "Available commands:\n";
  helpText += "time - Show current time\n";
  helpText += "time-enter - Enter time in format XX:XX:XX\n";
  helpText += "os - OS information\n";
  helpText += "unfi - System specifications\n";
  helpText += "temp - Show MCU/CPU temperature\n";
  server.send(200, "text/plain", helpText);
}

void handleOS() {
  String osInfo = "OS: N2 OS\n";
  osInfo += "Version: 1.0\n";
  osInfo += "Created by: Sliz\n";
  server.send(200, "text/plain", osInfo);
}

void handleUNFI() {
  String sysInfo = "Processor: ESP8266\n";
  sysInfo += "Free memory: " + String(ESP.getFreeHeap()) + " bytes\n";
  sysInfo += "Clock frequency: " + String(ESP.getCpuFreqMHz()) + " MHz\n";
  server.send(200, "text/plain", sysInfo);
}

void handleTemp() {
  float temperature = dht.readTemperature(); // Get temperature
  String tempInfo = "MCU/CPU Temperature: " + String(temperature) + " C\n";
  server.send(200, "text/plain", tempInfo);
}

void playTone() {
  analogWrite(BUZZER_PIN, 255);  // Use analogWrite instead of tone
  delay(200);
  analogWrite(BUZZER_PIN, 0);    // Stop sound
}
