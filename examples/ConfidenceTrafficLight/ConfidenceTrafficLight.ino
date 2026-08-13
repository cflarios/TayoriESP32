/*
 * ConfidenceTrafficLight — un semáforo de confianza para las respuestas de Tayori.
 *
 * En vez de decir QUÉ opción, dice si te puedes fiar de ella:
 *   - Verde: llegó una respuesta firme.
 *   - Ámbar: el modelo la marcó con `DUDA:` — da su mejor opción sin estar seguro.
 *   - Rojo:  no se pudo leer ninguna opción (p. ej. `NO SE VE:` en la captura).
 *
 * Es el mismo dato que usa QuizLeds (el flag `uncertain`), pero mirado desde el
 * otro lado: en un examen con penalización, saber si arriesgas importa tanto como
 * la letra. El LED se apaga solo a los pocos segundos para no quedarse clavado.
 *
 * Conexiones (LED + resistencia ~220Ω a GND):
 *   Verde → GPIO 25   Ámbar → GPIO 26   Rojo → GPIO 27
 */

#include <TayoriESP32.h>

const char* WIFI_SSID = "TU_WIFI";
const char* WIFI_PASS = "TU_PASSWORD";
const char* BROKER    = "mqtt://192.168.1.10:1883";
const char* TOPIC     = "tayori/answer";

const int LED_GREEN = 25;
const int LED_AMBER = 26;
const int LED_RED   = 27;

const unsigned long HOLD_MS = 6000;  // cuánto se queda encendido el semáforo

TayoriESP32 tayori;
unsigned long litSince = 0;
bool lit = false;

void allOff() {
  digitalWrite(LED_GREEN, LOW);
  digitalWrite(LED_AMBER, LOW);
  digitalWrite(LED_RED, LOW);
  lit = false;
}

void show(int pin) {
  digitalWrite(LED_GREEN, pin == LED_GREEN ? HIGH : LOW);
  digitalWrite(LED_AMBER, pin == LED_AMBER ? HIGH : LOW);
  digitalWrite(LED_RED,   pin == LED_RED   ? HIGH : LOW);
  lit = true;
  litSince = millis();
}

void onQuiz(const TayoriQuiz& q) {
  if (!q.option)        show(LED_RED);      // no se pudo leer la opción
  else if (q.uncertain) show(LED_AMBER);    // opción, pero con DUDA
  else                  show(LED_GREEN);    // respuesta firme

  Serial.printf("Opción %c%s\n", q.option ? q.option : '?',
                q.uncertain ? " (DUDA)" : "");
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_AMBER, OUTPUT);
  pinMode(LED_RED, OUTPUT);

  tayori.setWiFi(WIFI_SSID, WIFI_PASS);
  tayori.setBroker(BROKER);
  tayori.setTopic(TOPIC);
  tayori.onQuiz(onQuiz);
  Serial.println(tayori.begin() ? "Conectado" : "Fallo (reintenta en loop)");
}

void loop() {
  tayori.loop();

  if (lit && millis() - litSince > HOLD_MS) allOff();
}
