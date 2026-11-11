// Taller 1 - Reto 1: resistencia de pull-up interna configurada por codigo
// Pulsador entre el GPIO 27 y GND; se retira la resistencia externa de 10 kohm.
const int PULSADOR = 27;   // GPIO 27 (admite pull-up interna)
const int LED = 26;        // GPIO 26 (salida digital)

void setup() {
  pinMode(PULSADOR, INPUT_PULLUP);    // activa la resistencia interna hacia 3,3 V
  pinMode(LED, OUTPUT);
}

void loop() {
  if (digitalRead(PULSADOR) == LOW) {    // logica invertida: presionado = LOW
    digitalWrite(LED, HIGH);
  } else {
    digitalWrite(LED, LOW);
  }
}
