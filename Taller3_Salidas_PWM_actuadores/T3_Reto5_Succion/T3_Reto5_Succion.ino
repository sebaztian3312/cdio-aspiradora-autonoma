// Taller 3 - Reto 5: motor de succion en un canal independiente
// Traccion: dos canales del L293D a 20 kHz, alimentados por su propia fuente.
// Succion: su propio pin, su propio canal LEDC (otra frecuencia, otro temporizador),
// su propio transistor MOSFET con diodo de rueda libre y su propia fuente.

const int ENA = 19, IN1 = 18, IN2 = 17;   // motor izquierdo (canal A)
const int ENB = 16, IN3 = 4,  IN4 = 21;   // motor derecho (canal B)
// compuerta del MOSFET de succion (o LED que la representa)
const int SUC = 25;
const int RES = 8;
const uint32_t F_TRACCION = 20000;        // Hz
const uint32_t F_SUCCION  = 25000;        // Hz, temporizador distinto al de la traccion
const int NIVELES[] = {0, 90, 170, 255};  // apagado, bajo, medio, alto
int nivel = 0;
int velocidad = 180;

void motor(int en, int a, int b, int vel) {
  digitalWrite(a, vel > 0);
  digitalWrite(b, vel < 0);
  ledcWrite(en, abs(vel));
}

void mover(int izq, int der) {
  motor(ENA, IN1, IN2, izq);
  motor(ENB, IN3, IN4, der);
}

void succion(int n) {                     // la succion se cambia sin tocar la traccion
  nivel = constrain(n, 0, 3);
  ledcWrite(SUC, NIVELES[nivel]);
  Serial.printf("Succion nivel %d (duty %d/255)\n", nivel, NIVELES[nivel]);
}

void setup() {
  Serial.begin(115200);
  pinMode(IN1, OUTPUT); pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT); pinMode(IN4, OUTPUT);
  ledcAttach(ENA, F_TRACCION, RES);
  ledcAttach(ENB, F_TRACCION, RES);
  ledcAttach(SUC, F_SUCCION, RES);        // canal y temporizador propios
  mover(0, 0);
  succion(0);
  Serial.println("Traccion: w s a d q e x | Succion: + sube, - baja, 0 apaga");
}

void loop() {
  if (!Serial.available()) return;
  char c = Serial.read();
  int v = velocidad;
  switch (c) {
    case 'w': mover(v, v);        Serial.println("Avanzar");          break;
    case 's': mover(-v, -v);      Serial.println("Retroceder");       break;
    case 'a': mover(v / 3, v);    Serial.println("Girar izquierda");  break;
    case 'd': mover(v, v / 3);    Serial.println("Girar derecha");    break;
    case 'q': mover(-v, v);       Serial.println("Pivote izquierda"); break;
    case 'e': mover(v, -v);       Serial.println("Pivote derecha");   break;
    case 'x': mover(0, 0);        Serial.println("Detenido");         break;
    case '+': succion(nivel + 1); break;
    case '-': succion(nivel - 1); break;
    case '0': succion(0);         break;
  }
}
