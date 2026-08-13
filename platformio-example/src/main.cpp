/*
 * Punto de partida para PlatformIO — vuelca al monitor serie cada respuesta.
 *
 * Compila sin cablear nada: sirve para confirmar que el broker y el tema están
 * bien antes de conectar LEDs o pantallas. En Tayori, Ajustes → MQTT →
 * "Probar conexión" hace que aparezca aquí un mensaje de prueba.
 *
 * Para el resto de escenarios (LEDs, semáforo, OLED, JSON), copia el contenido
 * del ejemplo correspondiente de examples/ aquí y descomenta sus dependencias
 * en platformio.ini.
 */

#include <TayoriESP32.h>

const char* WIFI_SSID = "TU_WIFI";
const char* WIFI_PASS = "TU_PASSWORD";
const char* BROKER    = "mqtt://192.168.1.10:1883";
const char* TOPIC     = "tayori/answer";

TayoriESP32 tayori;

void onText(const String& text) {
  Serial.println("── Respuesta ──");
  Serial.println(text);
  Serial.println();
}

void setup() {
  Serial.begin(115200);
  tayori.setWiFi(WIFI_SSID, WIFI_PASS);
  tayori.setBroker(BROKER);
  tayori.setTopic(TOPIC);
  tayori.onText(onText);
  Serial.println(tayori.begin() ? "Conectado, esperando respuestas…"
                                : "No se pudo conectar (reintenta en loop)");
}

void loop() {
  tayori.loop();
}
