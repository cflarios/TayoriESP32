#include "TayoriESP32.h"
#include <WiFi.h>

/** Un broker que no reconecta antes de esto no se reintenta todavía. */
static const unsigned long RECONNECT_EVERY_MS = 5000;
/** Techo para la espera del WiFi al arrancar. */
static const unsigned long WIFI_TIMEOUT_MS = 20000;

TayoriESP32* TayoriESP32::_self = nullptr;

TayoriESP32::TayoriESP32() : _mqtt(_ownedNet) {}

TayoriESP32::TayoriESP32(Client& net) : _mqtt(net) {}

// ── Configuración ─────────────────────────────────────────────────────────────

void TayoriESP32::setWiFi(const char* ssid, const char* password) {
  _ssid = ssid;
  _pass = password;
}

void TayoriESP32::setBroker(const char* host, uint16_t port) {
  _host = host;
  _port = port;
  _mqtt.setServer(_host.c_str(), _port);
}

bool TayoriESP32::setBroker(const char* url) {
  String u(url);
  u.trim();
  // TLS necesita un WiFiClientSecure inyectado por el constructor; aquí no.
  if (u.startsWith("mqtts://")) return false;
  if (u.startsWith("mqtt://")) u = u.substring(7);
  else if (u.startsWith("tcp://")) u = u.substring(6);

  int slash = u.indexOf('/');
  if (slash != -1) u = u.substring(0, slash);

  int colon = u.indexOf(':');
  if (colon == -1) {
    setBroker(u.c_str(), 1883);
    return true;
  }
  String host = u.substring(0, colon);
  long port = u.substring(colon + 1).toInt();
  if (host.length() == 0 || port <= 0 || port > 65535) return false;
  setBroker(host.c_str(), (uint16_t)port);
  return true;
}

void TayoriESP32::setCredentials(const char* user, const char* pass) {
  _user = user;
  _mqttPass = pass;
}

void TayoriESP32::setTopic(const char* base) {
  String b(base);
  b.trim();
  // Recorta la barra final: `a//text` es un tema legal y DISTINTO en MQTT, así
  // que el ESP32 no vería lo que el broker publica en `a/text`. Misma regla que
  // mqttTopics() del lado de Tayori.
  while (b.endsWith("/")) b.remove(b.length() - 1);
  if (b.length() == 0) b = "tayori/answer";
  _topicBase = b;
  _topicText = b + "/text";
}

void TayoriESP32::setClientId(const char* id) {
  _clientId = id;
}

void TayoriESP32::setBufferSize(uint16_t bytes) {
  _bufferSize = bytes;
}

// ── Callbacks ─────────────────────────────────────────────────────────────────

void TayoriESP32::onText(TayoriTextCallback cb) { _onText = cb; }
void TayoriESP32::onQuiz(TayoriQuizCallback cb) { _onQuiz = cb; }
void TayoriESP32::onJson(TayoriJsonCallback cb) { _onJson = cb; }

// ── Ciclo de vida ─────────────────────────────────────────────────────────────

bool TayoriESP32::begin() {
  _self = this;

  if (_clientId.length() == 0) {
    // Estable por placa, único entre placas: dos clientes con el mismo id se
    // echan el uno al otro del broker en un bucle de reconexiones.
    _clientId = "tayori-esp32-" + String((uint32_t)ESP.getEfuseMac(), HEX);
  }

  _mqtt.setBufferSize(_bufferSize);
  _mqtt.setServer(_host.c_str(), _port);
  _mqtt.setCallback(staticCallback);

  if (!ensureWiFi()) return false;
  return reconnect();
}

void TayoriESP32::loop() {
  if (!_mqtt.connected()) {
    unsigned long now = millis();
    if (now - _lastReconnectAttempt >= RECONNECT_EVERY_MS) {
      _lastReconnectAttempt = now;
      reconnect();
    }
    return;
  }
  _mqtt.loop();
}

bool TayoriESP32::connected() {
  return _mqtt.connected();
}

// ── Interno ───────────────────────────────────────────────────────────────────

bool TayoriESP32::ensureWiFi() {
  if (WiFi.status() == WL_CONNECTED) return true;
  if (!_ssid) return false;  // WiFi lo gestiona el sketch: nada que hacer aquí.

  WiFi.mode(WIFI_STA);
  WiFi.begin(_ssid, _pass);
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < WIFI_TIMEOUT_MS) {
    delay(250);
  }
  return WiFi.status() == WL_CONNECTED;
}

bool TayoriESP32::reconnect() {
  if (!ensureWiFi()) return false;

  bool ok = _user ? _mqtt.connect(_clientId.c_str(), _user, _mqttPass)
                  : _mqtt.connect(_clientId.c_str());
  if (ok) subscribe();
  return ok;
}

void TayoriESP32::subscribe() {
  // QoS 1: no perder la respuesta que el usuario ya pagó. Nos suscribimos sólo a
  // lo que alguien vaya a leer, para no traer el JSON grande si nadie lo quiere.
  if (_onText || _onQuiz) _mqtt.subscribe(_topicText.c_str(), 1);
  if (_onJson) _mqtt.subscribe(_topicBase.c_str(), 1);
}

void TayoriESP32::staticCallback(char* topic, uint8_t* payload, unsigned int length) {
  if (_self) _self->dispatch(topic, payload, length);
}

void TayoriESP32::dispatch(char* topic, uint8_t* payload, unsigned int length) {
  String msg;
  msg.reserve(length + 1);
  for (unsigned int i = 0; i < length; i++) msg += (char)payload[i];

  String t(topic);
  if (t == _topicText) {
    if (_onText) _onText(msg);
    if (_onQuiz) _onQuiz(parseQuiz(msg));
  } else if (t == _topicBase) {
    if (_onJson) _onJson(msg);
  }
}

// ── Parseo del quiz ───────────────────────────────────────────────────────────

TayoriQuiz TayoriESP32::parseQuiz(const String& text) {
  TayoriQuiz q = { '\0', false };

  // Primera línea no vacía: en modo test Tayori pone ahí la opción.
  String line;
  int start = 0;
  while (start < (int)text.length()) {
    int nl = text.indexOf('\n', start);
    int end = (nl == -1) ? text.length() : nl;
    line = text.substring(start, end);
    line.trim();
    if (line.length() > 0) break;
    start = end + 1;
  }
  if (line.length() == 0) return q;

  String upper = line;
  upper.toUpperCase();

  // `DUDA:` — el modelo no está seguro y aun así da su mejor opción. Se anota y
  // se quita del principio para no estorbar la búsqueda de la letra.
  if (upper.startsWith("DUDA:")) {
    q.uncertain = true;
    line = line.substring(5);
    line.trim();
    upper = line;
    upper.toUpperCase();
  }

  // `NO SE VE:` — no se leían todas las opciones en la captura. Sin letra.
  if (upper.startsWith("NO SE VE")) return q;

  // La letra de opción va marcada con ')' o '.' ("B)" o "1. B)"), así que el
  // "1" de la numeración no se confunde con una opción.
  for (int i = 0; i + 1 < (int)line.length(); i++) {
    char c = line.charAt(i);
    char u = (c >= 'a' && c <= 'z') ? (char)(c - 32) : c;
    if (u >= 'A' && u <= 'D') {
      char next = line.charAt(i + 1);
      if (next == ')' || next == '.' || next == ':') {
        q.option = u;
        return q;
      }
    }
  }

  // Respaldo: una línea que es sólo la letra ("B").
  if (line.length() == 1) {
    char c = line.charAt(0);
    char u = (c >= 'a' && c <= 'z') ? (char)(c - 32) : c;
    if (u >= 'A' && u <= 'D') q.option = u;
  }
  return q;
}
