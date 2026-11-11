// Taller 1 - Reto 3: cada pulsacion cambia el estado del LED (encendido <-> apagado)
const int PULSADOR = 27;
const int LED = 26;
const int PRESIONADO = LOW;

int lecturaAnterior = !PRESIONADO;   // arranca en HIGH (suelto)
bool estadoLed = false;              // LED inicialmente apagado

void setup() {
  pinMode(PULSADOR, INPUT_PULLUP);
  pinMode(LED, OUTPUT);
}

void loop() {
  int lectura = digitalRead(PULSADOR);

  // deteccion de flanco: estaba suelto (HIGH) y ahora esta presionado (LOW)
  if (lectura == PRESIONADO && lecturaAnterior != PRESIONADO) {
    estadoLed = !estadoLed;          // invierte el estado guardado
    digitalWrite(LED, estadoLed);    // y lo aplica al LED
  }
  lecturaAnterior = lectura;         // guarda la lectura para el siguiente ciclo
}
