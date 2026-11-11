// Taller 1 - Reto 2: cada pulsacion enciende el LED durante 5 segundos
const int PULSADOR = 27;
const int LED = 26;
const int PRESIONADO = LOW;                // logica invertida por la pull-up interna
const unsigned long T_ENCENDIDO = 5000;    // 5000 ms = 5 s

void setup() {
  pinMode(PULSADOR, INPUT_PULLUP);
  pinMode(LED, OUTPUT);
}

void loop() {
  if (digitalRead(PULSADOR) == PRESIONADO) {
    digitalWrite(LED, HIGH);               // 1. enciende el LED al presionar
    delay(T_ENCENDIDO);                    // 2. lo mantiene encendido 5 s
    digitalWrite(LED, LOW);                // 3. lo apaga

    while (digitalRead(PULSADOR) == PRESIONADO) {
      delay(10);                           // espera a que se suelte el pulsador
    }
    delay(50);                             // filtro antirrebote al soltar
  }
}
