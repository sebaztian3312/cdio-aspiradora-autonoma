// Taller 2 - Reto 1: voltaje del potenciometro en el Monitor Serie
// ESP32: ADC de 12 bits (0 a 4095) referido a 3,3 V
// Expresion equivalente: Vin = N_ADC * 3,3 / 4095

const int analogInPin = 34;   // cursor del potenciometro en GPIO 34 (ADC1_CH6)
int valorADC = 0;             // cuentas leidas del ADC
float Vin = 0;                // voltaje en el pin, en voltios

void setup() {
  Serial.begin(115200);       // comunicacion serial a 115200 bps
}

void loop() {
  valorADC = analogRead(analogInPin);   // lee la entrada analogica
  Vin = valorADC * 3.3 / 4095.0;        // convierte las cuentas a voltios
  Serial.print("Vin = ");
  Serial.print(Vin, 3);                 // tres decimales
  Serial.println(" V");
  delay(200);
}
