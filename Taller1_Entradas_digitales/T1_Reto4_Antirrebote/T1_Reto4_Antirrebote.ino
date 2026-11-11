// Taller 1 - Reto 4: reto 3 con antirrebote por tiempo (millis)
const int PULSADOR = 27;                     // GPIO 27 del ESP32
const int LED = 26;                          // GPIO 26 del ESP32
const int PRESIONADO = LOW;                  // logica invertida por INPUT_PULLUP

bool estadoLed = false;
int estadoPulsador = !PRESIONADO;            // estado estable (validado) del pulsador
int ultimaLectura = !PRESIONADO;             // lectura instantanea anterior
unsigned long ultimoTiempoRebote = 0;
const unsigned long TIEMPO_ANTIRREBOTE = 50; // 50 ms para filtrar el ruido mecanico

void setup() {
  pinMode(PULSADOR, INPUT_PULLUP);
  pinMode(LED, OUTPUT);
  digitalWrite(LED, estadoLed);
}

void loop() {
  int lectura = digitalRead(PULSADOR);

  // 1. si la lectura cambia respecto a la anterior, se reinicia el temporizador
  if (lectura != ultimaLectura) {
    ultimoTiempoRebote = millis();
  }

  // 2. solo se acepta el cambio si la lectura lleva 50 ms sin variar
  if ((millis() - ultimoTiempoRebote) > TIEMPO_ANTIRREBOTE) {
    if (lectura != estadoPulsador) {
      estadoPulsador = lectura;

      // 3. si el estado estable es PRESIONADO (flanco de bajada), conmuta el LED
      if (estadoPulsador == PRESIONADO) {
        estadoLed = !estadoLed;
        digitalWrite(LED, estadoLed);
      }
    }
  }
  ultimaLectura = lectura;
}
