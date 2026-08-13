/*
 * RawText — imprime por el puerto serie cada respuesta de Tayori.
 *
 * El "hola mundo" de la librería: se suscribe al texto crudo (<base>/text) y lo
 * vuelca al monitor serie. Útil para comprobar que el broker y el tema están
 * bien antes de conectar nada físico. En Ajustes → MQTT, "Probar conexión" hace
 * que aparezca aquí un mensaje de prueba.
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
