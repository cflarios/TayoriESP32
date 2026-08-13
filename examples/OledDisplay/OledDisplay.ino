/*
 * OledDisplay — muestra en una OLED el texto de la respuesta de Tayori.
 *
 * La pantalla del móvil (el "espejo" de Tayori) resuelve leer una solución larga
 * fuera de la pantalla compartida. Esto es la versión cacharro: una OLED de 0,96"
 * pegada al monitor que enseña la última respuesta sin salir en la grabación.
 *
 * Para un quiz cabe de sobra; para una solución de código sólo verás el principio
 * (una OLED de 128x64 son ~8 líneas). Sube el buffer si esperas textos largos.
 *
 * Hardware: OLED SSD1306 128x64 por I2C (dirección típica 0x3C).
 *   SDA → GPIO 21   SCL → GPIO 22   VCC → 3V3   GND → GND
 *
 * Librerías: Adafruit_SSD1306 y Adafruit_GFX (gestor de librerías del IDE).
 */

#include <TayoriESP32.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

const char* WIFI_SSID = "TU_WIFI";
const char* WIFI_PASS = "TU_PASSWORD";
const char* BROKER    = "mqtt://192.168.1.10:1883";
const char* TOPIC     = "tayori/answer";

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_ADDR 0x3C

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
TayoriESP32 tayori;

void banner(const char* msg) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println(msg);
  display.display();
}

void onText(const String& text) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setTextWrap(true);   // parte las líneas largas solo
  display.setCursor(0, 0);
  display.print(text);         // lo que no cabe en 8 líneas se pierde: es un vistazo
  display.display();
  Serial.println(text);
}

void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22);
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println("No se encontró la OLED en 0x3C");
    for (;;) delay(1000);
  }
  banner("Conectando...");

  tayori.setWiFi(WIFI_SSID, WIFI_PASS);
  tayori.setBroker(BROKER);
  tayori.setTopic(TOPIC);
  tayori.onText(onText);
  banner(tayori.begin() ? "Listo. Esperando\nrespuestas..." : "Fallo de conexion");
}

void loop() {
  tayori.loop();
}
