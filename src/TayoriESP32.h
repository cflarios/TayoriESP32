/*
 * TayoriESP32 — recibe en un ESP32 las respuestas que Tayori publica por MQTT.
 *
 * Tayori (https://github.com/... asistente de IA para reuniones) publica cada
 * respuesta TERMINADA en un broker MQTT, en dos temas:
 *
 *   <base>        JSON: { id, trigger, question, answer, providerId, model, at }
 *   <base>/text   Sólo el texto de la respuesta, en crudo.
 *
 * El segundo existe para cacharros como éste: te suscribes y lees la respuesta
 * sin meter un parser de JSON en la placa. Esta librería envuelve PubSubClient
 * para que, en cuatro líneas, un ESP32 reaccione a lo que dice el asistente.
 *
 * El caso que la motiva: un cuestionario (quiz) donde el agente responde la
 * opción correcta (A, B, C o D) y la placa enciende el LED que le toca.
 *
 * Contrato que replica del lado del broker (ver bridge/mqtt.ts de Tayori):
 *   - QoS 1 al suscribirse: no perder la respuesta que ya se pagó.
 *   - Sin retención por parte del emisor: al arrancar no llega la de ayer.
 *   - Sólo respuestas completas: nunca fragmentos, errores ni cancelaciones.
 *
 * Dependencia: PubSubClient (Nick O'Leary). Para `mqtts://` (TLS), pasa un
 * WiFiClientSecure al constructor; ver el ejemplo y setBroker().
 *
 * MIT — igual que Tayori.
 */

#ifndef TAYORI_ESP32_H
#define TAYORI_ESP32_H

#include <Arduino.h>
#include <WiFiClient.h>
#include <PubSubClient.h>

/** La opción de un cuestionario, tal y como la extrae onQuiz(). */
struct TayoriQuiz {
  /** 'A'..'D', o '\0' si no se pudo leer ninguna opción en la primera línea. */
  char option;
  /** El modelo marcó la respuesta con `DUDA:`: da su mejor opción sin estar seguro. */
  bool uncertain;
};

typedef void (*TayoriTextCallback)(const String& text);
typedef void (*TayoriQuizCallback)(const TayoriQuiz& quiz);
typedef void (*TayoriJsonCallback)(const String& json);

class TayoriESP32 {
public:
  /** Usa un WiFiClient interno (broker en claro, `mqtt://`). */
  TayoriESP32();

  /**
   * Inyecta el cliente de red — por ejemplo un WiFiClientSecure para `mqtts://`.
   * La librería no toma posesión: el objeto debe vivir tanto como la instancia.
   */
  explicit TayoriESP32(Client& net);

  // ── Configuración (llamar antes de begin) ──────────────────────────────────

  void setWiFi(const char* ssid, const char* password);

  /** Broker por host y puerto. El puerto MQTT en claro habitual es 1883. */
  void setBroker(const char* host, uint16_t port = 1883);

  /**
   * Broker desde una URL como la que usa Tayori: `mqtt://host:1883`.
   * Devuelve false si la URL es `mqtts://` (necesita un cliente TLS: usa el
   * constructor con Client& y setBroker(host, port) a mano) o no se puede leer.
   */
  bool setBroker(const char* url);

  /** Usuario y contraseña del broker. Omítelo si el broker es anónimo. */
  void setCredentials(const char* user, const char* pass);

  /** Tema base; debe coincidir con el de Tayori. Por defecto "tayori/answer". */
  void setTopic(const char* base);

  /** Id de cliente MQTT. Por defecto "tayori-esp32-<chip>". Debe ser único. */
  void setClientId(const char* id);

  /**
   * Tamaño del buffer de PubSubClient. Por defecto 1024: el de la librería (256)
   * corta las respuestas de código. El texto de un quiz cabe de sobra; súbelo si
   * te suscribes al JSON completo o esperas soluciones largas.
   */
  void setBufferSize(uint16_t bytes);

  // ── Callbacks (registra los que necesites) ─────────────────────────────────

  /** Texto crudo de cada respuesta (tema <base>/text). */
  void onText(TayoriTextCallback cb);

  /** Opción de cuestionario ya extraída de la primera línea (tema <base>/text). */
  void onQuiz(TayoriQuizCallback cb);

  /** JSON completo con metadatos (tema <base>). Tú lo parseas. */
  void onJson(TayoriJsonCallback cb);

  // ── Ciclo de vida ──────────────────────────────────────────────────────────

  /** Conecta el WiFi y el broker, y se suscribe a lo que haga falta. */
  bool begin();

  /** Llama a esto en loop(): mantiene la conexión y procesa los mensajes. */
  void loop();

  /** true si el broker está conectado ahora mismo. */
  bool connected();

  /** Extrae la opción (A/B/C/D) del texto de un quiz. Estática y sin estado. */
  static TayoriQuiz parseQuiz(const String& text);

private:
  bool ensureWiFi();
  bool reconnect();
  void subscribe();
  void dispatch(char* topic, uint8_t* payload, unsigned int length);
  static void staticCallback(char* topic, uint8_t* payload, unsigned int length);

  WiFiClient _ownedNet;
  PubSubClient _mqtt;

  const char* _ssid = nullptr;
  const char* _pass = nullptr;
  String _host;
  uint16_t _port = 1883;
  const char* _user = nullptr;
  const char* _mqttPass = nullptr;
  String _topicBase = "tayori/answer";
  String _topicText = "tayori/answer/text";
  String _clientId;
  uint16_t _bufferSize = 1024;

  TayoriTextCallback _onText = nullptr;
  TayoriQuizCallback _onQuiz = nullptr;
  TayoriJsonCallback _onJson = nullptr;

  unsigned long _lastReconnectAttempt = 0;

  /** PubSubClient no lleva un puntero de usuario al callback, así que se enruta
   *  por aquí. Implica una sola instancia activa a la vez. */
  static TayoriESP32* _self;
};

#endif  // TAYORI_ESP32_H
