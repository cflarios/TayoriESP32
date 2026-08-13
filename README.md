# TayoriESP32

Recibe en un **ESP32** las respuestas que [Tayori](https://github.com/cflarios/jarvis-job)
publica por **MQTT**, y reacciona a ellas con hardware.

Tayori es un asistente de IA para reuniones y entrevistas. Cuando resuelve un
cuestionario en pantalla (`Ctrl+Alt+Q`), responde una pregunta hablada o resuelve
código, puede **publicar esa respuesta en un broker MQTT**. Esta librería es el
otro lado del cable: se suscribe, te da el texto ya masticado y — el caso que la
motiva — extrae la opción de un test (**A, B, C o D**) para que enciendas un LED.

```
Tayori (PC)  ──publish──►  Broker MQTT  ──subscribe──►  ESP32 (esta librería)
```

## El caso de ejemplo

Tienes un formulario tipo test en pantalla. Pulsas `Ctrl+Alt+Q`, Tayori lee la
pregunta y publica la opción correcta. El ESP32 enciende el LED de esa letra.

```cpp
#include <TayoriESP32.h>

TayoriESP32 tayori;

void onQuiz(const TayoriQuiz& q) {
  // q.option es 'A', 'B', 'C', 'D' — o '\0' si no se pudo leer.
  // q.uncertain es true si el modelo marcó la respuesta con "DUDA:".
  encenderLed(q.option);
}

void setup() {
  tayori.setWiFi("mi-wifi", "clave");
  tayori.setBroker("mqtt://192.168.1.10:1883");
  tayori.setTopic("tayori/answer");
  tayori.onQuiz(onQuiz);
  tayori.begin();
}

void loop() {
  tayori.loop();  // imprescindible: mantiene la conexión y procesa mensajes
}
```

El sketch completo, con los cuatro LEDs y el parpadeo para las dudas, está en
[`examples/QuizLeds`](examples/QuizLeds/QuizLeds.ino).

## Instalación

1. **Dependencia:** instala **PubSubClient** (Nick O'Leary) desde el gestor de
   librerías del IDE de Arduino. Para el ejemplo del JSON, además **ArduinoJson**.
2. Copia esta carpeta en tu carpeta `libraries/` de Arduino, o usa
   *Sketch → Include Library → Add .ZIP Library*.
3. Selecciona una placa ESP32 en el gestor de placas.

## Configuración en Tayori

En la app: **Ajustes → MQTT**.

- Enciende MQTT y pon la **URL del broker**, el **usuario/contraseña** (si los
  tiene) y el **tema base** — el mismo que pongas en el ESP32.
- **"Probar conexión"** publica un mensaje de prueba: si tu ESP32 ya está
  suscrito, lo verás llegar. Es la forma de confirmar que el tema coincide antes
  de jugártela con una pregunta real.

> Ten en cuenta lo que dice el propio Tayori: MQTT **saca tus respuestas de la
> app**. Un broker sin usuario ni TLS es un tablón de anuncios; usa `mqtts://`
> fuera de tu red (ver [TLS](#tls-mqtts) abajo).

## La API

| Método | Para qué |
|---|---|
| `setWiFi(ssid, pass)` | Credenciales WiFi. Omítelo si conectas el WiFi tú mismo. |
| `setBroker("mqtt://host:puerto")` | Broker desde URL. Devuelve `false` si es `mqtts://`. |
| `setBroker(host, puerto)` | Broker por host y puerto sueltos. |
| `setCredentials(user, pass)` | Usuario y contraseña del broker. Omítelo si es anónimo. |
| `setTopic("tayori/answer")` | Tema base. Debe coincidir con el de Tayori. |
| `setClientId(id)` | Id de cliente MQTT. Por defecto uno único por chip. |
| `setBufferSize(bytes)` | Buffer de PubSubClient. Por defecto 1024. |
| `onText(cb)` | `cb(const String& text)` — texto crudo (`<base>/text`). |
| `onQuiz(cb)` | `cb(const TayoriQuiz& q)` — opción A/B/C/D ya extraída. |
| `onJson(cb)` | `cb(const String& json)` — JSON completo con metadatos (`<base>`). |
| `begin()` | Conecta WiFi + broker y se suscribe. |
| `loop()` | Llámalo en `loop()`: mantiene la conexión y procesa mensajes. |
| `connected()` | `true` si el broker está conectado ahora. |
| `TayoriESP32::parseQuiz(text)` | Extrae la opción de un texto. Estático, sin estado. |

La librería **sólo se suscribe a lo que vas a leer**: si registras `onQuiz`/`onText`
va al tema `/text`; si registras `onJson`, al tema base. Reconecta sola en `loop()`.

## Los dos temas de Tayori

| Tema | Contenido | Cuándo usarlo |
|---|---|---|
| `<base>/text` | Sólo el texto de la respuesta | Lo normal en una placa: sin parser de JSON |
| `<base>` | JSON `{ id, trigger, question, answer, providerId, model, at }` | Cuando necesitas el contexto (p. ej. filtrar por `trigger`) |

El campo `trigger` distingue de dónde salió la respuesta: `quiz`, `code`, `auto`,
`hotkey`, `manual-input`. El ejemplo [`FullAnswerJson`](examples/FullAnswerJson/FullAnswerJson.ino)
reacciona sólo a los `quiz` e ignora el resto — algo que el tema `/text` no
permite, porque ahí todo llega igual.

## Lo que hereda del contrato de Tayori

Tayori publica con **QoS 1 y sin retención**, y **sólo respuestas terminadas**.
La librería se suscribe también con **QoS 1**. En la práctica eso significa:

- **Llega una respuesta por pregunta**, entera, no los fragmentos del streaming.
- **No llegan errores ni respuestas canceladas.** Tu placa nunca actúa sobre un
  fallo disfrazado de respuesta.
- **Una placa que arranca por la mañana no ejecuta la respuesta de ayer** (sin
  retención).

## Detalles que evitan un mal rato

- **El buffer por defecto de PubSubClient son 256 bytes**, que corta cualquier
  respuesta larga. La librería lo sube a **1024**. El texto de un quiz cabe de
  sobra; para el JSON completo o soluciones de código, sube más con
  `setBufferSize()`.
- **Una sola instancia a la vez.** PubSubClient no lleva un puntero de usuario en
  su callback, así que el enrutado usa una instancia estática. Un ESP32 con dos
  objetos `TayoriESP32` no funcionará.
- **El tema base sin barra final.** `a//text` es un tema legal y *distinto* en
  MQTT; `setTopic()` recorta la barra igual que hace Tayori, para que los dos
  lados hablen del mismo tema.
- **`onQuiz` sólo entiende el formato de test de Tayori**: la letra marcada con
  `)` o `.` en la primera línea (`B)`, `1. B)`). Reconoce `DUDA:` (lo marca en
  `uncertain`) y `NO SE VE:` (devuelve opción `'\0'`).

## TLS (mqtts)

Para un broker en internet, usa `mqtts://`. `setBroker(url)` con `mqtts://`
devuelve `false` a propósito: hace falta un cliente TLS. Pásalo por el constructor
y configura el puerto a mano:

```cpp
#include <WiFiClientSecure.h>

WiFiClientSecure net;
TayoriESP32 tayori(net);

void setup() {
  net.setInsecure();           // o net.setCACert(...) con el certificado real
  tayori.setWiFi("wifi", "clave");
  tayori.setBroker("mi-broker.com", 8883);
  tayori.setCredentials("user", "pass");
  tayori.setTopic("tayori/answer");
  tayori.onQuiz(onQuiz);
  tayori.begin();
}
```

## Más ideas de qué colgar del ESP32

El quiz → LED es sólo el ejemplo mínimo. Con `onText`, `onQuiz` y `onJson` da
para más:

- **Semáforo de confianza**: LED verde si la respuesta es firme, ámbar si viene
  con `DUDA:`. El campo `uncertain` ya lo distingue.
- **Pantalla del texto**: una OLED/e-paper que muestre la respuesta completa,
  para leer una solución larga fuera de la pantalla compartida.
- **Servo o matriz** que apunte/dibuje la letra de la opción.
- **Relé / domótica**: publica hacia Home Assistant y dispara una escena.
- **Zumbador** que avise cuando llega una respuesta, para no mirar la pantalla.
- **Filtrar por `trigger`**: que el cacharro reaccione sólo a `quiz` y no a lo
  que se habla en la reunión (ver el ejemplo del JSON).

## Ejemplos

| Ejemplo | Qué muestra |
|---|---|
| [`RawText`](examples/RawText/RawText.ino) | El "hola mundo": vuelca cada respuesta al monitor serie. |
| [`QuizLeds`](examples/QuizLeds/QuizLeds.ino) | Cuatro LEDs A/B/C/D + parpadeo para las dudas. |
| [`ConfidenceTrafficLight`](examples/ConfidenceTrafficLight/ConfidenceTrafficLight.ino) | Semáforo verde/ámbar/rojo según la confianza de la respuesta. |
| [`OledDisplay`](examples/OledDisplay/OledDisplay.ino) | Muestra el texto de la respuesta en una OLED SSD1306. |
| [`FullAnswerJson`](examples/FullAnswerJson/FullAnswerJson.ino) | Lee los metadatos y filtra por `trigger`. |

## Licencia

MIT — igual que Tayori.
