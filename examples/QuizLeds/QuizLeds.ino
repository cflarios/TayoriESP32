/*
 * QuizLeds — enciende el LED de la opción que responde Tayori.
 *
 * Tayori resuelve un cuestionario en pantalla (Ctrl+Alt+Q) y publica la opción
 * correcta por MQTT. Este ESP32 se suscribe, lee la letra (A/B/C/D) y enciende
 * el LED que le toca. Cuatro LEDs, uno por opción.
 *
 * Si el modelo marcó la respuesta con `DUDA:` (inseguro), el LED parpadea en
 * vez de quedarse fijo: en un examen con penalización conviene saber si arriesgas.
 *
 * Conexiones (LED + resistencia ~220Ω a GND en cada pin):
 *   A → GPIO 25   B → GPIO 26   C → GPIO 27   D → GPIO 14
 *
 * Necesita en Tayori: Ajustes → MQTT encendido, el mismo broker y tema que aquí.
 */

#include <TayoriESP32.h>

// ── Ajusta esto ────────────────────────────────────────────────────────────────
const char* WIFI_SSID = "TU_WIFI";
const char* WIFI_PASS = "TU_PASSWORD";
const char* BROKER    = "mqtt://192.168.1.10:1883";  // el de Ajustes → MQTT
const char* MQTT_USER = "";                            // "" si es anónimo
const char* MQTT_PASS = "";
const char* TOPIC     = "tayori/answer";               // el tema base de Tayori
// ────────────────────────────────────────────────────────────────────────────────

const int LED_A = 25;
const int LED_B = 26;
const int LED_C = 27;
const int LED_D = 14;

int ledFor(char option) {
  switch (option) {
    case 'A': return LED_A;
    case 'B': return LED_B;
    case 'C': return LED_C;
    case 'D': return LED_D;
    default:  return -1;
  }
}

void allOff() {
  digitalWrite(LED_A, LOW);
  digitalWrite(LED_B, LOW);
  digitalWrite(LED_C, LOW);
  digitalWrite(LED_D, LOW);
}

TayoriESP32 tayori;

// El parpadeo del "DUDA" no puede bloquear el loop() con delay(): se lleva por
// tiempo. Guardamos qué pin parpadea y desde cuándo.
int blinkPin = -1;
unsigned long blinkStart = 0;

void onQuiz(const TayoriQuiz& quiz) {
  int pin = ledFor(quiz.option);
  Serial.printf("Respuesta: %c%s\n", quiz.option ? quiz.option : '?',
                quiz.uncertain ? " (DUDA)" : "");

  allOff();
  blinkPin = -1;

  if (pin < 0) return;  // no se pudo leer la opción: todo apagado

  if (quiz.uncertain) {
    blinkPin = pin;      // el loop() lo hace parpadear
    blinkStart = millis();
  } else {
    digitalWrite(pin, HIGH);
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_A, OUTPUT);
  pinMode(LED_B, OUTPUT);
  pinMode(LED_C, OUTPUT);
  pinMode(LED_D, OUTPUT);
  allOff();

  tayori.setWiFi(WIFI_SSID, WIFI_PASS);
  tayori.setBroker(BROKER);
  if (strlen(MQTT_USER)) tayori.setCredentials(MQTT_USER, MQTT_PASS);
  tayori.setTopic(TOPIC);
  tayori.onQuiz(onQuiz);

  Serial.print("Conectando… ");
  Serial.println(tayori.begin() ? "listo" : "fallo (reintenta en loop)");
}

void loop() {
  tayori.loop();

  if (blinkPin >= 0) {
    bool on = ((millis() - blinkStart) / 300) % 2 == 0;  // 300 ms encendido/apagado
    digitalWrite(blinkPin, on ? HIGH : LOW);
  }
}
