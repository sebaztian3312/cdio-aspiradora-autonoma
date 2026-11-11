// Taller 3 - Reto 3: rampa de aceleracion para los dos motores (no bloqueante)
// Los comandos fijan una velocidad objetivo; la velocidad aplicada se acerca a ella
// en pasos pequenos, tambien al frenar y al invertir el sentido

const int ENA = 19, IN1 = 18, IN2 = 17;   // motor izquierdo (canal A)
const int ENB = 16, IN3 = 4,  IN4 = 21;   // motor derecho (canal B)
const int FREQ = 1000, RES = 8;
const int PASO = 15;                      // cambio de duty por paso
const unsigned long T_PASO = 60;          // ms entre pasos: 0 a 255 en menos de 1,1 s
int velocidad = 180;

int objIzq = 0, objDer = 0;               // velocidades pedidas
int actIzq = 0, actDer = 0;               // velocidades aplicadas
unsigned long tPaso = 0, tInicio = 0;

void motor(int en, int a, int b, int vel) {
  digitalWrite(a, vel > 0);
  digitalWrite(b, vel < 0);
  ledcWrite(en, abs(vel));
}

int acercar(int act, int obj) {           // un paso de la rampa
  if (act < obj) return min(act + PASO, obj);
  if (act > obj) return max(act - PASO, obj);
  return act;
}

void pedir(int izq, int der, const char* txt) {
  objIzq = izq; objDer = der; tInicio = millis();
  Serial.printf("%s -> objetivo izq = %d, der = %d\n", txt, izq, der);
}

void setup() {
  Serial.begin(115200);
  pinMode(IN1, OUTPUT); pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT); pinMode(IN4, OUTPUT);
  ledcAttach(ENA, FREQ, RES);
  ledcAttach(ENB, FREQ, RES);
  Serial.println("w avanzar | s retroceder | a/d girar | q/e pivote | x detener");
}

void loop() {
  if (Serial.available()) {
    char c = Serial.read();
    int v = velocidad;
    if (c == 'w') pedir(v, v, "Avanzar");
    if (c == 's') pedir(-v, -v, "Retroceder");
    if (c == 'a') pedir(v / 3, v, "Girar a la izquierda");
    if (c == 'd') pedir(v, v / 3, "Girar a la derecha");
    if (c == 'q') pedir(-v, v, "Pivote izquierda");
    if (c == 'e') pedir(v, -v, "Pivote derecha");
    if (c == 'x') pedir(0, 0, "Detener");
  }

  if (millis() - tPaso >= T_PASO) {       // la rampa avanza sin bloquear el loop
    tPaso = millis();
    int nIzq = acercar(actIzq, objIzq);
    int nDer = acercar(actDer, objDer);
    if (nIzq != actIzq || nDer != actDer) {
      actIzq = nIzq; actDer = nDer;
      motor(ENA, IN1, IN2, actIzq);
      motor(ENB, IN3, IN4, actDer);
      Serial.printf("t = %4lu ms | izq = %4d | der = %4d\n", millis() - tInicio, actIzq, actDer);
    }
  }
}
