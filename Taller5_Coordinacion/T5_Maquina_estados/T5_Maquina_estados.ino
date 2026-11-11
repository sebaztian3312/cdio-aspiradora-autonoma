// Taller 5 - Coordinacion de entradas y salidas (ESP32, arduino-esp32 3.x)
// Maquina de estados sin delay(): REPOSO, ACTIVO, PAUSA y ALERTA_BATERIA.
// El potenciometro representa la bateria, el pulsador es el boton de arranque y los
// LED son los indicadores. Integra los ultrasonicos (Taller 4), los motores (Taller 3)
// y el bumper por interrupcion.
// Comandos por el monitor serie:
//   d -> alterna la version con delay(200), solo para comparar (procedimiento 4)
//   s -> activa o desactiva los sensores y los motores (reto 5)
//   m -> compara la respuesta al bumper con sondeo y con interrupcion (reto 7)

// ---------------- pines ----------------
const int BOTON = 27, BUMPER = 33;               // entradas con pull-up interna
const int BAT = 34;                              // potenciometro que representa la bateria
const int LED1 = 26, LED2 = 13;                  // LED 1: estado, LED 2: bateria
const int TRIG_F = 32, ECHO_F = 35, TRIG_L = 22, ECHO_L = 36;   // ultrasonicos
const int ENA = 19, IN1 = 18, IN2 = 17;          // motor izquierdo
const int ENB = 16, IN3 = 4, IN4 = 21;           // motor derecho
const int PRUEBA = 23;                           // unido al bumper: simula choques (reto 7)

// ---------------- parametros ----------------
const int ADC_AVISO = 2286;                      // aviso del reto 6 del Taller 2: enciende el LED 2
const int ADC_CORTE = 1000;                      // por debajo: entra a ALERTA_BATERIA
const int ADC_SALIDA = 1200;                     // por encima: sale de ALERTA_BATERIA (histeresis)
const unsigned long T_PARPADEO = 500;            // 500 ms encendido + 500 ms apagado = 1 Hz
const unsigned long T_REBOTE = 50;               // ms que el pulsador debe estar estable
const unsigned long T_SENSOR = 60;               // ms entre disparos de los ultrasonicos
const unsigned long TIMEOUT_ECO = 12000;         // us: espera maxima del eco (~2 m)
const float D_PARAR = 20.0, D_SEGUIR = 25.0;     // cm: se detiene y vuelve a avanzar
const int VEL = 180;                             // ciclo util de avance (0 a 255)

enum Estado { REPOSO, ACTIVO, PAUSA, ALERTA_BATERIA };
const char* NOMBRE[] = {"REPOSO", "ACTIVO", "PAUSA", "ALERTA_BATERIA"};
Estado estado = REPOSO;

// ---------------- variables ----------------
volatile bool choque = false;                    // la escribe la interrupcion del bumper
volatile bool sondeoPendiente = false;           // el sondeo aun no ha visto el ultimo choque
volatile unsigned long tChoque = 0;              // micros() en el instante del choque

int adcBat = 4095;
float distF = 999, distL = 999;
bool obstaculo = false, sensores = true, conDelay = false, led1 = false;
unsigned long tParpadeo = 0, tSensor = 0;
bool turnoFrontal = true;
char txt[48];

// medicion del tiempo de ciclo (procedimiento 4)
unsigned long tLoopAnterior = 0, peorCiclo = 0, tVentana = 0;

// comparacion sondeo contra interrupcion (reto 7)
const int N_PRUEBAS = 20;                        // choques simulados por medicion
bool midiendo = false, lineaAbajo = false;
int pruebasHechas = 0, vistosSondeo = 0;
unsigned long tPrueba = 0, peorSondeo = 0, sumaSondeo = 0;

// ---------------- interrupcion ----------------
void IRAM_ATTR isrBumper() {                     // corta: solo guarda el instante y avisa
  if (!choque) { tChoque = micros(); choque = true; sondeoPendiente = true; }
}

// ---------------- registro de transiciones ----------------
void registrar(const char* evento) {
  Serial.printf("%7lu ms | %-14s | %s\n", millis(), NOMBRE[estado], evento);
}
void cambiar(Estado nuevo, const char* evento) {
  Serial.printf("%7lu ms | %-14s -> %-14s | %s\n", millis(), NOMBRE[estado], NOMBRE[nuevo], evento);
  estado = nuevo;
  tParpadeo = millis();
}

// ---------------- actuadores ----------------
void motores(int duty) {                         // duty > 0 avanza, 0 detiene
  digitalWrite(IN1, duty > 0); digitalWrite(IN2, LOW);
  digitalWrite(IN3, duty > 0); digitalWrite(IN4, LOW);
  ledcWrite(ENA, duty); ledcWrite(ENB, duty);
}

// ---------------- entradas ----------------
// antirrebote sin bloqueo: devuelve true una sola vez por pulsacion
bool leerPulsacion() {
  static int estable = HIGH, anterior = HIGH;
  static unsigned long tCambio = 0;
  int lectura = digitalRead(BOTON);
  if (lectura != anterior) { anterior = lectura; tCambio = millis(); }
  if (millis() - tCambio >= T_REBOTE && lectura != estable) {
    estable = lectura;
    if (estable == LOW) return true;             // pulsacion validada
  }
  return false;
}

float medir(int trig, int echo) {
  digitalWrite(trig, LOW);  delayMicroseconds(2);
  digitalWrite(trig, HIGH); delayMicroseconds(10);
  digitalWrite(trig, LOW);
  unsigned long t = pulseIn(echo, HIGH, TIMEOUT_ECO);
  if (t == 0) return 999;                        // sin eco: nada al frente
  return t / 58.0;
}

// los dos ultrasonicos se disparan por turnos cada 60 ms (Taller 4, reto 4)
void leerSensores() {
  if (!sensores || millis() - tSensor < T_SENSOR) return;
  tSensor = millis();
  if (turnoFrontal) distF = medir(TRIG_F, ECHO_F);
  else              distL = medir(TRIG_L, ECHO_L);
  turnoFrontal = !turnoFrontal;
}

// ---------------- comandos por serie ----------------
void comandos() {
  if (!Serial.available()) return;
  char c = Serial.read();
  if (c == 'd') {
    conDelay = !conDelay;
    peorCiclo = 0; tVentana = millis();
    Serial.printf("Version con delay(200): %s\n", conDelay ? "SI" : "NO");
  } else if (c == 's') {
    sensores = !sensores;
    peorCiclo = 0; tVentana = millis();
    Serial.printf("Sensores y motores: %s\n", sensores ? "ACTIVADOS" : "DESACTIVADOS");
  } else if (c == 'm') {
    midiendo = true; lineaAbajo = false; sondeoPendiente = false;
    pruebasHechas = 0; vistosSondeo = 0; peorSondeo = 0; sumaSondeo = 0; tPrueba = millis();
  }
}

// ---------------- reto 7: sondeo contra interrupcion ----------------
// La interrupcion guarda el instante exacto del choque. El sondeo revisa el pin una vez
// por vuelta del loop; la diferencia entre los dos es lo que tarda el sondeo en enterarse.
void reto7() {
  if (!midiendo) return;
  if (sondeoPendiente && digitalRead(BUMPER) == LOW) {       // sondeo: una lectura por vuelta
    sondeoPendiente = false;
    unsigned long retraso = micros() - tChoque;
    sumaSondeo += retraso; vistosSondeo++;
    if (retraso > peorSondeo) peorSondeo = retraso;
  }
  unsigned long t = millis();
  if (!lineaAbajo && t - tPrueba >= 150) {                   // nuevo choque simulado
    if (pruebasHechas == N_PRUEBAS) {
      Serial.printf("%7lu ms | reto 7 | %d choques, sensores: %s\n",
                    millis(), N_PRUEBAS, sensores ? "SI" : "NO");
      Serial.printf("          interrupcion: registra el choque en el instante del flanco\n");
      Serial.printf("          sondeo: lo ve %lu us despues en promedio y %lu us en el peor caso\n",
                    sumaSondeo / max(vistosSondeo, 1), peorSondeo);
      midiendo = false;
      return;
    }
    pinMode(PRUEBA, OUTPUT); digitalWrite(PRUEBA, LOW);      // baja la linea del bumper
    lineaAbajo = true; tPrueba = t; pruebasHechas++;
  } else if (lineaAbajo && t - tPrueba >= 100) {             // la suelta a los 100 ms
    pinMode(PRUEBA, INPUT);
    lineaAbajo = false; tPrueba = t;
  }
}

// ---------------- programa principal ----------------
void setup() {
  Serial.begin(115200);
  pinMode(BOTON, INPUT_PULLUP);
  pinMode(BUMPER, INPUT_PULLUP);
  pinMode(PRUEBA, INPUT);                        // alta impedancia: no afecta al bumper
  pinMode(LED1, OUTPUT); pinMode(LED2, OUTPUT);
  pinMode(TRIG_F, OUTPUT); pinMode(TRIG_L, OUTPUT);
  pinMode(ECHO_F, INPUT);  pinMode(ECHO_L, INPUT);
  pinMode(IN1, OUTPUT); pinMode(IN2, OUTPUT); pinMode(IN3, OUTPUT); pinMode(IN4, OUTPUT);
  ledcAttach(ENA, 20000, 8);
  ledcAttach(ENB, 20000, 8);
  motores(0);
  attachInterrupt(digitalPinToInterrupt(BUMPER), isrBumper, FALLING);
  Serial.println("Taller 5 - comandos: d = version con delay, s = sensores, m = medir bumper");
  registrar("encendido");
  tVentana = millis();
}

void loop() {
  // procedimiento 4: tiempo entre dos vueltas del loop y peor caso en 30 s
  unsigned long ahoraUs = micros();
  unsigned long ciclo = ahoraUs - tLoopAnterior;
  if (tLoopAnterior != 0 && ciclo > peorCiclo) peorCiclo = ciclo;
  tLoopAnterior = ahoraUs;
  if (millis() - tVentana >= 30000) {
    Serial.printf("%7lu ms | peor ciclo del loop en 30 s: %lu us (delay: %s, sensores: %s)\n",
                  millis(), peorCiclo, conDelay ? "SI" : "NO", sensores ? "SI" : "NO");
    peorCiclo = 0; tVentana = millis();
  }

  reto7();                                       // solo actua despues del comando m

  // 1) prioridad: bateria, evaluada antes que cualquier otro evento (reto 3)
  adcBat = analogRead(BAT);
  if (estado != ALERTA_BATERIA && adcBat < ADC_CORTE) {
    motores(0);
    snprintf(txt, sizeof txt, "bateria baja: %d cuentas", adcBat);
    cambiar(ALERTA_BATERIA, txt);
  } else if (estado == ALERTA_BATERIA && adcBat > ADC_SALIDA) {   // histeresis (reto 4)
    snprintf(txt, sizeof txt, "bateria recuperada: %d cuentas", adcBat);
    cambiar(REPOSO, txt);
  }

  // 2) bumper atendido por interrupcion (reto 6)
  if (choque) {
    choque = false;
    if (!midiendo) {                             // los choques simulados del reto 7 no cuentan
      if (estado == ACTIVO) { motores(0); cambiar(PAUSA, "choque del bumper"); }
      else registrar("choque del bumper");
    }
  }

  // 3) pulsador con antirrebote (reto 2)
  if (leerPulsacion()) {
    if (estado == REPOSO)      cambiar(ACTIVO, "pulsacion");
    else if (estado == ACTIVO) cambiar(PAUSA, "pulsacion");
    else if (estado == PAUSA)  cambiar(ACTIVO, "pulsacion");
    else registrar("pulsacion ignorada: bateria baja");   // el pulsador no saca de la alerta
  }

  // 4) sensores y motores (reto 5): un obstaculo detiene los motores, el LED sigue
  leerSensores();
  if (estado == ACTIVO && sensores) {
    float d = min(distF, distL);
    if (!obstaculo && d < D_PARAR) {
      obstaculo = true;
      snprintf(txt, sizeof txt, "obstaculo a %.1f cm: motores detenidos", d);
      registrar(txt);
    } else if (obstaculo && d > D_SEGUIR) {
      obstaculo = false;
      registrar("camino libre: motores en marcha");
    }
    motores(obstaculo ? 0 : VEL);
  } else {
    motores(0);
  }

  // 5) salidas de cada estado
  switch (estado) {
    case REPOSO:         digitalWrite(LED1, LOW);  break;
    case PAUSA:          digitalWrite(LED1, HIGH); break;
    case ALERTA_BATERIA: digitalWrite(LED1, LOW);  break;
    case ACTIVO:                                  // parpadeo a 1 Hz con millis()
      if (millis() - tParpadeo >= T_PARPADEO) {
        tParpadeo = millis();
        led1 = !led1;
        digitalWrite(LED1, led1);
      }
      break;
  }
  digitalWrite(LED2, estado == ALERTA_BATERIA || adcBat < ADC_AVISO);

  comandos();
  if (conDelay) delay(200);                      // solo para la comparacion del procedimiento 4
}
