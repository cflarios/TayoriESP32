# Ejemplo PlatformIO

Proyecto mínimo listo para compilar con [PlatformIO](https://platformio.org/) en
VSCode. Úsalo como plantilla.

## Cómo

1. Abre esta carpeta en VSCode con la extensión *PlatformIO IDE* instalada.
2. Edita `src/main.cpp`: pon tu WiFi, la URL del broker y el tema (los mismos que
   en Tayori → Ajustes → MQTT).
3. Ajusta `board` en `platformio.ini` a tu placa si no es una `esp32dev`.
4. Compila y sube con los botones ✓ → → de la barra inferior, y abre el monitor
   serie (🔌) a 115200 baudios.

`main.cpp` es el ejemplo `RawText`: no necesita cablear nada. Para LEDs, semáforo,
OLED o JSON, copia aquí el ejemplo correspondiente de [`../examples`](../examples)
y descomenta sus dependencias en `platformio.ini`.

## Desarrollo local de la librería

Si vas a editar TayoriESP32 a la vez, en lugar de la URL de git clónala dentro de
una carpeta `lib/` de este proyecto: PlatformIO detecta ahí las librerías y usa esa
copia en vez de la descargada.
