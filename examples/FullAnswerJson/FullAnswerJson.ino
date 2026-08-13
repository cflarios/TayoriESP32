/*
 * FullAnswerJson — usa los metadatos de la respuesta, no sólo el texto.
 *
 * Tayori publica en el tema base un JSON con contexto:
 *   { id, trigger, question, answer, providerId, model, at }
 *
 * `trigger` te dice de dónde salió la respuesta:
 *   "quiz"          → un cuestionario resuelto en pantalla (Ctrl+Alt+Q)
 *   "code"          → una solución de código (Ctrl+Alt+C)
 *   "auto"/"hotkey" → una pregunta hablada en la reunión
 *   "manual-input"  → algo que se escribió a mano
 *
 * Aquí se reacciona SÓLO a los quiz, ignorando el resto — algo que el tema
 * /text (texto pelado) no permite distinguir. Ojo con el buffer: el JSON es más
 * grande que el texto solo, y una solución de código puede ser enorme.
 *
 * Requiere la librería ArduinoJson (Benoit Blanchon), instalable desde el
 * gestor de librerías del IDE.
 */

#include <TayoriESP32.h>
#include <ArduinoJson.h>

const char* WIFI_SSID = "TU_WIFI";
const char* WIFI_PASS = "TU_PASSWORD";
const char* BROKER    = "mqtt://192.168.1.10:1883";
const char* TOPIC     = "tayori/answer";

TayoriESP32 tayori;

void onJson(const String& json) {
  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, json);
  if (err) {
    Serial.printf("JSON ilegible: %s\n", err.c_str());
    return;
  }

  const char* trigger = doc["trigger"] | "";
  if (strcmp(trigger, "quiz") != 0) {
    Serial.printf("Ignorada (trigger=%s)\n", trigger);
    return;
  }

  const char* answer = doc["answer"] | "";
  const char* model  = doc["model"]  | "";
  Serial.printf("Quiz resuelto por %s:\n%s\n\n", model, answer);

  // La misma extracción de la opción que hace onQuiz(), disponible como estática.
  TayoriQuiz quiz = TayoriESP32::parseQuiz(answer);
  if (quiz.option) {
    Serial.printf("→ Opción %c%s\n", quiz.option, quiz.uncertain ? " (DUDA)" : "");
  }
}

void setup() {
  Serial.begin(115200);
  tayori.setWiFi(WIFI_SSID, WIFI_PASS);
  tayori.setBroker(BROKER);
  tayori.setTopic(TOPIC);
  tayori.setBufferSize(4096);  // el JSON no cabe en el buffer por defecto
  tayori.onJson(onJson);
  Serial.println(tayori.begin() ? "Conectado" : "Fallo de conexión");
}

void loop() {
  tayori.loop();
}
