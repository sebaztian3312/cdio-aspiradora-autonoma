// Taller 2 - Reto 3: LED controlado por el voltaje del potenciometro
// Comparacion con un umbral y, como solucion al parpadeo, histeresis

const int POT = 34;              // cursor del potenciometro (ADC1_CH6)
const int LED = 18;              // LED con resistencia de 330 Ohm en serie
const float UMBRAL = 1.65;       // voltaje de conmutacion (V), mitad del rango
const float BANDA = 0.05;        // media banda de histeresis (V)
const bool USAR_HISTERESIS = true;   // false = comparacion simple con un solo umbral
// el potenciometro simulado no tiene ruido: se suman +/-20 cuentas
const bool SIMULAR_RUIDO = true;

bool ledEncendido = false;
int cambios = 0;                 // cuantas veces ha conmutado el LED

void setup() {
  Serial.begin(115200);
  pinMode(LED, OUTPUT);
  digitalWrite(LED, LOW);
}

void loop() {
  int lectura = analogRead(POT);
  if (SIMULAR_RUIDO) lectura += random(-20, 21);   // ruido agregado para ver el efecto
  float Vin = lectura * 3.3 / 4095.0;

  bool nuevoEstado;
  if (USAR_HISTERESIS) {
    // enciende por encima de UMBRAL + BANDA y apaga por debajo de UMBRAL - BANDA
    if (!ledEncendido && Vin > UMBRAL + BANDA)      nuevoEstado = true;
    else if (ledEncendido && Vin < UMBRAL - BANDA)  nuevoEstado = false;
    else                                            nuevoEstado = ledEncendido;
  } else {
    nuevoEstado = (Vin > UMBRAL);                   // un solo umbral
  }

  if (nuevoEstado != ledEncendido) {
    ledEncendido = nuevoEstado;
    digitalWrite(LED, ledEncendido ? HIGH : LOW);
    cambios++;
  }

  Serial.printf("Vin = %.3f V | LED = %s | cambios = %d\n",
                Vin, ledEncendido ? "ON " : "OFF", cambios);
  delay(100);
}
