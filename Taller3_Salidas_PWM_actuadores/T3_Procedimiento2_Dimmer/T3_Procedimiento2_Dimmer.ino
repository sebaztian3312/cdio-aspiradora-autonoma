// Taller 3 - Procedimiento 2: dimmer, la intensidad del LED sigue al potenciometro
const int POT = 34;          // cursor del potenciometro (ADC1)
const int LED = 26;          // LED con resistencia de 330 ohm
const int FREQ = 5000;       // 5 kHz: sin parpadeo visible
const int RES = 8;           // 8 bits: ciclo util de 0 a 255

unsigned long tImpresion = 0;

void setup() {
  Serial.begin(115200);
  ledcAttach(LED, FREQ, RES);   // arduino-esp32 3.x: asigna canal y temporizador al pin
}

void loop() {
  int lectura = analogRead(POT);                // 0 a 4095
  int duty = map(lectura, 0, 4095, 0, 255);     // 12 bits del ADC -> 8 bits del PWM
  ledcWrite(LED, duty);

  if (millis() - tImpresion >= 200) {           // imprime 5 veces por segundo
    tImpresion = millis();
    Serial.printf("ADC = %4d  ->  intensidad = %3d/255 (%3d %%)\n",
                  lectura, duty, duty * 100 / 255);
  }
}
