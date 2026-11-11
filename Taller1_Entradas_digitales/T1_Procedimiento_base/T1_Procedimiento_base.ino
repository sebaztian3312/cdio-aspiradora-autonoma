// Taller 1 - Procedimiento: codigo de referencia adaptado al ESP32
// Pulsador entre 3V3 y el GPIO 27, con resistencia de 10 kohm del pin a GND (pull-down externo).
const int PULSADOR = 27;   // pulsador en el GPIO 27
const int LED = 26;        // LED en el GPIO 26 (con resistencia de 330 ohm)

void setup() {
  pinMode(PULSADOR, INPUT);           // entrada con pull-down externo de 10 kohm
  pinMode(LED, OUTPUT);
}

void loop() {
  if (digitalRead(PULSADOR) == HIGH) {   // presionado = 3,3 V en el pin
    digitalWrite(LED, HIGH);             // enciende el LED
  } else {
    digitalWrite(LED, LOW);              // de lo contrario lo apaga
  }
}
