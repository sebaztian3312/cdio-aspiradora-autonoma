// Taller 3 - Reto 2: dos motores en configuracion diferencial, comandos por el Monitor Serie
// Driver puente H: L293D en la simulacion (se conecta igual que el L298N)
// En la simulacion cada motor es el "DC Motor" de Velxio, que muestra las rpm y el sentido de giro

const int ENA = 19, IN1 = 18, IN2 = 17;   // motor izquierdo (canal A)
const int ENB = 16, IN3 = 4,  IN4 = 21;   // motor derecho (canal B)
const int FREQ = 1000;                    // frecuencia PWM (Hz)
const int RES = 8;                        // resolucion: duty de 0 a 255
int velocidad = 180;                      // duty que usan los comandos

// vel > 0 avanza, vel < 0 retrocede, vel = 0 detiene
void motor(int en, int a, int b, int vel) {
  digitalWrite(a, vel > 0);
  digitalWrite(b, vel < 0);
  ledcWrite(en, abs(vel));
}

void mover(int izq, int der) {
  motor(ENA, IN1, IN2, izq);
  motor(ENB, IN3, IN4, der);
}

void setup() {
  Serial.begin(115200);
  pinMode(IN1, OUTPUT); pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT); pinMode(IN4, OUTPUT);
  ledcAttach(ENA, FREQ, RES);             // API de arduino-esp32 3.x
  ledcAttach(ENB, FREQ, RES);
  mover(0, 0);
  Serial.println("w avanzar | s retroceder | a girar izq | d girar der");
  Serial.println("q pivote izq | e pivote der | x detener | 1-9 velocidad");
}

void loop() {
  if (!Serial.available()) return;
  char c = Serial.read();
  int v = velocidad;
  switch (c) {
    case 'w': mover(v, v);         Serial.println("Avanzar");                    break;
    case 's': mover(-v, -v);       Serial.println("Retroceder");                 break;
    case 'a': mover(v / 3, v);     Serial.println("Girar a la izquierda");       break;
    case 'd': mover(v, v / 3);     Serial.println("Girar a la derecha");         break;
    case 'q': mover(-v, v);        Serial.println("Girar sobre su eje (izq)");   break;
    case 'e': mover(v, -v);        Serial.println("Girar sobre su eje (der)");   break;
    case 'x': mover(0, 0);         Serial.println("Detenido");                   break;
    default:
      if (c >= '1' && c <= '9') {
        velocidad = map(c - '0', 1, 9, 60, 255);
        Serial.printf("Velocidad = %d / 255\n", velocidad);
      }
  }
}
