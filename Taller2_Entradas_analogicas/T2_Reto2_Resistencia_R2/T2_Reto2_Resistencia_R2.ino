// Taller 2 - Reto 2: valor de la resistencia R2 (Figura 5) en kOhm
// El potenciometro es un divisor: Vin = Vs * R2 / (R1 + R2), con R1 + R2 = RPOT
// Despejando: R2 = RPOT * Vin / Vs

const int analogInPin = 34;   // cursor del potenciometro en GPIO 34 (ADC1_CH6)
const float VS = 3.3;         // alimentacion del potenciometro en ESP32 (V)
const float RPOT = 10.0;      // valor nominal del potenciometro (kOhm)

void setup() {
  Serial.begin(115200);
}

void loop() {
  int valorADC = analogRead(analogInPin);   // cuentas del ADC (0 a 4095)
  float Vin = valorADC * VS / 4095.0;       // voltaje en el cursor
  float R2 = RPOT * Vin / VS;               // resistencia entre cursor y GND
  float R1 = RPOT - R2;                     // resistencia entre VCC y cursor
  Serial.printf("Vin = %.3f V | R2 = %.2f kOhm | R1 = %.2f kOhm\n", Vin, R2, R1);
  delay(200);
}
