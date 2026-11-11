// Taller 3 - Procedimiento 4: velocidad y sentido de un motor con el potenciometro
// Centro de la perilla = detenido; hacia un lado avanza y hacia el otro retrocede.
const int POT = 34;
const int ENA = 19, IN1 = 18, IN2 = 17;   // canal A del puente H (plan de pines)
const int CENTRO = 2048;                  // perilla en la mitad del recorrido
const int ZONA_MUERTA = 200;              // +-200 cuentas (~5 % del recorrido) = detenido

unsigned long tImpresion = 0;

void setup() {
  Serial.begin(115200);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  ledcAttach(ENA, 20000, 8);              // 20 kHz: fuera del rango audible (reto 4)
}

void loop() {
  int lectura = analogRead(POT);
  int desvio = lectura - CENTRO;          // positivo: avanzar, negativo: retroceder
  int duty = 0;
  const char* sentido = "DETENIDO";

  if (desvio > ZONA_MUERTA) {                         // avanzar
    digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
    duty = map(desvio, ZONA_MUERTA, 4095 - CENTRO, 0, 255);
    sentido = "AVANZA";
  } else if (desvio < -ZONA_MUERTA) {                 // retroceder
    digitalWrite(IN1, LOW); digitalWrite(IN2, HIGH);
    duty = map(-desvio, ZONA_MUERTA, CENTRO, 0, 255);
    sentido = "RETROCEDE";
  } else {                                            // zona muerta: motor detenido
    digitalWrite(IN1, LOW); digitalWrite(IN2, LOW);
  }
  duty = constrain(duty, 0, 255);
  ledcWrite(ENA, duty);

  if (millis() - tImpresion >= 200) {
    tImpresion = millis();
    Serial.printf("ADC = %4d | desvio = %5d | duty = %3d | %s\n", lectura, desvio, duty, sentido);
  }
}
