// Taller 2 - Reto 5: divisor de tension para medir una bateria de 12 V
// R1 = 47 kOhm (lado bateria), R2 = 10 kOhm (lado GND), salida al GPIO 34 (ADC1)

const int PIN_BAT = 34;    // salida del divisor
const float R1 = 47000.0;  // resistencia del lado de la bateria
const float R2 = 10000.0;  // resistencia del lado de GND
const float FACTOR = (R1 + R2) / R2;   // deshace la division
const int VENTANA = 8;     // promedio movil del reto 4

int historial[VENTANA];
int indice = 0;
long suma = 0;

float promedioMovil(int nueva) {
  suma -= historial[indice];
  historial[indice] = nueva;
  suma += nueva;
  indice = (indice + 1) % VENTANA;
  return (float)suma / VENTANA;
}

void setup() {
  Serial.begin(115200);
  int primera = analogRead(PIN_BAT);
  for (int i = 0; i < VENTANA; i++) historial[i] = primera;
  suma = (long)primera * VENTANA;
}

void loop() {
  int cuentas = analogRead(PIN_BAT);
  float filtradas = promedioMovil(cuentas);
  float vPin = filtradas * 3.3 / 4095.0;   // expresion del ADC del ESP32
  float vBat = vPin * FACTOR;              // voltaje real de la bateria
  Serial.printf("ADC = %4d | Vpin = %.2f V | Vbat = %.2f V\n", cuentas, vPin, vBat);
  delay(500);
}
