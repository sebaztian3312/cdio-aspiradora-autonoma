// Taller 3 - Reto 4: frecuencia del PWM
// Por defecto el core arduino-esp32 usa 1 kHz con 8 bits para analogWrite().
// El comando 'f' alterna entre 1 kHz (audible) y 20 kHz (por encima del oido humano)

const int ENA = 19, IN1 = 18, IN2 = 17;   // motor izquierdo (canal A)
const int ENB = 16, IN3 = 4,  IN4 = 21;   // motor derecho (canal B)
const int RES = 8;                        // 8 bits: duty de 0 a 255
const uint32_t F_BAJA = 1000;             // valor por defecto del core
const uint32_t F_ALTA = 20000;            // fuera del rango audible
uint32_t frecuencia = F_BAJA;
int velocidad = 128;                      // 50 % de ciclo util, facil de ver en el osciloscopio

void motor(int en, int a, int b, int vel) {
  digitalWrite(a, vel > 0);
  digitalWrite(b, vel < 0);
  ledcWrite(en, abs(vel));
}

void setup() {
  Serial.begin(115200);
  pinMode(IN1, OUTPUT); pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT); pinMode(IN4, OUTPUT);
  ledcAttach(ENA, frecuencia, RES);
  ledcAttach(ENB, frecuencia, RES);
  motor(ENA, IN1, IN2, velocidad);        // ambos motores avanzan al 50 %
  motor(ENB, IN3, IN4, velocidad);
  Serial.printf("PWM a %lu Hz, duty %d/255. Enviar 'f' para cambiar la frecuencia\n",
                frecuencia, velocidad);
}

void loop() {
  if (Serial.available() && Serial.read() == 'f') {
    frecuencia = (frecuencia == F_BAJA) ? F_ALTA : F_BAJA;
    ledcChangeFrequency(ENA, frecuencia, RES);   // reconfigura el temporizador del canal
    ledcChangeFrequency(ENB, frecuencia, RES);
    ledcWrite(ENA, velocidad);
    ledcWrite(ENB, velocidad);
    Serial.printf("Frecuencia PWM = %lu Hz (periodo = %.1f us)\n", frecuencia, 1e6 / frecuencia);
  }
}
