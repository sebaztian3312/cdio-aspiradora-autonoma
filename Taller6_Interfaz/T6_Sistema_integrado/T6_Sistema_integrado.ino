// Taller 6 - Interfaz e interaccion (ESP32, arduino-esp32 3.x)
// Un solo boton: pulsacion corta = iniciar/pausar, larga (> 2 s) = modo prueba.
// Senalizacion con LED verde, LED rojo y zumbador; menu de diagnostico por serie;
// parada de emergencia; registro de eventos con marca de tiempo.
// Se construye sobre la maquina de estados del Taller 5 y no usa delay().

#include <esp_task_wdt.h>

// ---------------- pines ----------------
const int BOTON = 27, BUMPER = 33, PARO = 14;          // entradas con pull-up interna
const int BAT = 34, PISO = 39;                          // entradas analogicas (ADC1)
const int TRIG_F = 32, ECHO_F = 35, TRIG_L = 22, ECHO_L = 36; // ultrasonicos
const int LED_V = 26, LED_R = 13, ZUMB = 23;            // interfaz
const int ENA = 19, IN1 = 18, IN2 = 17;                 // motor izquierdo
const int ENB = 16, IN3 = 4,  IN4 = 21;                 // motor derecho
const int SUC = 25;                                     // succion (MOSFET o LED)

// ---------------- parametros ----------------
const unsigned long T_LARGA = 2000;     // ms para pulsacion larga
const unsigned long T_REBOTE = 30;      // ms de estabilidad del boton
const int ADC_AVISO = 2155;             // 9,9 V: aviso de bateria baja
const int ADC_CORTE = 1959;             // 9,0 V: entra a ALERTA_BATERIA
const int ADC_SALIDA = 2221;            // 10,2 V: sale de ALERTA (histeresis)
const float K_BAT = 3.3 / 4095.0 * 5.7; // cuentas -> voltios de bateria (divisor 47k/10k)
const int UMBRAL_PISO = 3000;           // por encima: no hay piso (calibrar, Taller 4)
const float D_OBST = 20.0, D_LIBRE = 25.0;   // cm, con histeresis
const unsigned long TIMEOUT_ECO = 12000;     // us: limita el bloqueo de pulseIn (~2 m)
const float SIN_ECO = 999.0;                 // valor cuando no vuelve el eco
const int VEL = 180, VEL_SUC = 170;

// ---------------- estados ----------------
enum Estado { REPOSO, ACTIVO, PAUSA, OBSTACULO, ALERTA_BATERIA, PRUEBA, EMERGENCIA };
const char* NOMBRE[] = {"REPOSO", "ACTIVO", "PAUSA", "OBSTACULO",
                        "ALERTA_BATERIA", "PRUEBA", "EMERGENCIA"};
Estado estado = REPOSO;
unsigned long tEstado = 0, tLibre = 0;
bool registro = true;

// ---------------- entradas ----------------
volatile bool choque = false;           // lo escribe la interrupcion del bumper
int adcBat = 4095, adcPiso = 0;
float distF = SIN_ECO, distL = SIN_ECO;
bool avisoBat = false, paroActivo = false;

// boton: lectura estable y duracion de la pulsacion
bool btnEstable = HIGH, btnUltima = HIGH, largaAtendida = false;
unsigned long tCambio = 0, tPresion = 0;

// ---------------- salidas ----------------
int velObj = 0, velAct = 0;             // rampa del Taller 3
int pitidos = 0, fPitido = 2000;        // pitidos pendientes y su frecuencia
unsigned long tPitido = 0, durPitido = 0;
bool zumbando = false;

// ---------------- modo prueba ----------------
int paso = 0; unsigned long tPaso = 0; int fallas = 0; bool soloActuadores = false;

void pitar(int n, int f, unsigned long dur);

void IRAM_ATTR isrBumper() { choque = true; }   // solo marca el evento; nada de Serial aqui

// ================= registro de eventos =================
void registrar(const char* evento, const char* valor) {
  if (!registro) return;
  Serial.printf("%6lu ms | %-9s | %s | %s\n", millis(), NOMBRE[estado], evento, valor);
}

void cambiar(Estado nuevo, const char* evento, const char* valor) {
  if (nuevo == estado) return;
  if (registro)
    Serial.printf("%6lu ms | %-9s | %s | %s -> %s\n",
                  millis(), NOMBRE[estado], evento, valor, NOMBRE[nuevo]);
  estado = nuevo; tEstado = millis();
  switch (nuevo) {                      // sonido de entrada a cada estado
    case ACTIVO:         pitar(1, 2000, 80);  break;
    case PAUSA:          pitar(2, 2000, 80);  break;
    case OBSTACULO:      pitar(1, 800, 250);  break;
    case ALERTA_BATERIA: pitar(3, 1200, 150); break;
    case REPOSO:         pitar(1, 1500, 40);  break;
    default: break;
  }
  if (nuevo == PRUEBA) { paso = 0; tPaso = millis(); fallas = 0; }
}

// ================= actuadores =================
void motor(int en, int a, int b, int vel) {
  digitalWrite(a, vel > 0);
  digitalWrite(b, vel < 0);
  ledcWrite(en, abs(vel));
}
void mover(int izq, int der) { motor(ENA, IN1, IN2, izq); motor(ENB, IN3, IN4, der); }
void cortarTodo() { velObj = velAct = 0; mover(0, 0); ledcWrite(SUC, 0); }

void pitar(int n, int f, unsigned long dur) {
  pitidos = n; fPitido = f; durPitido = dur; tPitido = 0; zumbando = false;
}

void atenderZumbador() {                // reproduce los pitidos sin bloquear
  unsigned long ahora = millis();
  if (pitidos == 0 && !zumbando) return;
  if (ahora - tPitido < durPitido) return;
  tPitido = ahora;
  if (zumbando) { ledcWriteTone(ZUMB, 0); zumbando = false; pitidos--; }
  else if (pitidos > 0) { ledcWriteTone(ZUMB, fPitido); zumbando = true; }
}

// ================= sensores =================
float medir(int trig, int echo) {
  digitalWrite(trig, LOW);  delayMicroseconds(2);
  digitalWrite(trig, HIGH); delayMicroseconds(10);
  digitalWrite(trig, LOW);
  unsigned long t = pulseIn(echo, HIGH, TIMEOUT_ECO);
  if (t == 0) return SIN_ECO;           // sin eco = nada al frente, NO 0 cm (ver reto 3)
  return t / 58.0;
}

void leerSensores() {
  static unsigned long tBat = 0, tUs = 0; static bool turnoF = true;
  static long suma = 0; static int hist[8]; static int idx = 0; static bool lleno = false;
  unsigned long ahora = millis();
  if (ahora - tBat >= 50) {             // bateria con promedio movil de 8 (Taller 2)
    tBat = ahora;
    int n = analogRead(BAT);
    if (!lleno) { for (int i = 0; i < 8; i++) hist[i] = n; suma = 8L * n; lleno = true; }
    suma += n - hist[idx]; hist[idx] = n; idx = (idx + 1) % 8;
    adcBat = suma / 8;
    adcPiso = analogRead(PISO);
  }
  if (ahora - tUs >= 60) {              // multiplexado temporal de los ultrasonicos (Taller 4)
    tUs = ahora;
    if (turnoF) distF = medir(TRIG_F, ECHO_F); else distL = medir(TRIG_L, ECHO_L);
    turnoF = !turnoF;
  }
  paroActivo = digitalRead(PARO) == HIGH;   // contacto NC a GND: abierto o cable cortado = paro
}

// ================= boton: corta / larga =================
// devuelve 1 = corta, 2 = larga, 0 = nada
int leerBoton() {
  bool lectura = digitalRead(BOTON);
  unsigned long ahora = millis();
  if (lectura != btnUltima) { btnUltima = lectura; tCambio = ahora; }
  if (ahora - tCambio >= T_REBOTE && lectura != btnEstable) {
    btnEstable = lectura;
    if (btnEstable == LOW) { tPresion = ahora; largaAtendida = false; }
    else if (!largaAtendida) return 1;          // se solto antes de 2 s
  }
  if (btnEstable == LOW && !largaAtendida && ahora - tPresion >= T_LARGA) {
    largaAtendida = true;                       // la larga se reconoce sin esperar a soltar
    return 2;
  }
  return 0;
}

// ================= senalizacion =================
void senalizar() {
  unsigned long t = millis();
  bool v = false, r = false;
  switch (estado) {
    case REPOSO:         v = (t % 2000) < 100;                break;  // destello cada 2 s
    case ACTIVO:         v = (t % 1000) < 500;                break;  // 1 Hz
    case PAUSA:          v = true;                            break;  // fijo
    case OBSTACULO:      v = (t % 1000) < 500; r = true;      break;
    case ALERTA_BATERIA: r = (t % 1000) < 500;                break;
    case PRUEBA:         v = (t % 500) < 250;  r = !v;        break;  // alternados
    case EMERGENCIA:     r = (t % 250) < 125;                 break;  // 4 Hz
  }
  if (avisoBat && (estado == REPOSO || estado == ACTIVO || estado == PAUSA))
    r = (t % 2000) > 1900;              // destello rojo corto: bateria baja
  digitalWrite(LED_V, v);
  digitalWrite(LED_R, r);
  static bool alarma = false;           // alarma intermitente de emergencia (2,5 Hz)
  bool on = estado == EMERGENCIA && (t % 400) < 200;
  if (on != alarma) { alarma = on; ledcWriteTone(ZUMB, on ? 2500 : 0); }
  // en ALERTA_BATERIA, un pitido cada 10 s
  if (estado == ALERTA_BATERIA && pitidos == 0 && !zumbando && (t - tEstado) % 10000 < 20)
    pitar(1, 1200, 150);
}

// ================= modo prueba =================
void resultado(const char* que, bool ok, const char* detalle) {
  if (!ok) fallas++;
  Serial.printf("  [%s] %-22s %s\n", ok ? " OK " : "FALLA", que, detalle);
}

void modoPrueba() {
  if (millis() - tPaso < 700) return;   // cada paso dura 0,7 s
  tPaso = millis();
  char txt[48];
  int p = paso++;
  if (soloActuadores && p >= 1 && p <= 8) { p = 9; paso = 10; }
  mover(0, 0); ledcWrite(SUC, 0);
  if (p <= 3) { digitalWrite(LED_V, LOW); digitalWrite(LED_R, LOW); }
  switch (p) {
    case 0: Serial.println(soloActuadores ? "\n--- PRUEBA DE ACTUADORES ---"
                                          : "\n--- MODO PRUEBA ---"); break;
    case 1: digitalWrite(LED_V, HIGH);
            resultado("LED verde", true, "accionado (verificar a la vista)"); break;
    case 2: digitalWrite(LED_R, HIGH);
            resultado("LED rojo", true, "accionado (verificar a la vista)"); break;
    case 3: pitar(1, 2000, 300);
            resultado("Zumbador", true, "accionado (verificar al oido)"); break;
    case 4: snprintf(txt, sizeof txt, "%d cuentas = %.2f V", adcBat, adcBat * K_BAT);
            resultado("Bateria", adcBat > ADC_AVISO, txt); break;
    case 5: snprintf(txt, sizeof txt, distF >= SIN_ECO ? "sin eco" : "%.1f cm", distF);
            resultado("Ultrasonico frontal", distF < SIN_ECO, txt); break;
    case 6: snprintf(txt, sizeof txt, distL >= SIN_ECO ? "sin eco" : "%.1f cm", distL);
            resultado("Ultrasonico lateral", distL < SIN_ECO, txt); break;
    case 7: snprintf(txt, sizeof txt, "%d cuentas (%s)", adcPiso,
                     adcPiso > UMBRAL_PISO ? "sin piso" : "piso");
            resultado("Sensor de piso", adcPiso > 50 && adcPiso < 4045, txt); break;
    case 8: resultado("Bumper", digitalRead(BUMPER) == HIGH,
                      digitalRead(BUMPER) == HIGH ? "libre" : "presionado o atascado"); break;
    case 9: mover(VEL, 0);  resultado("Motor izquierdo", true, "adelante (verificar giro)"); break;
    case 10: mover(-VEL, 0); resultado("Motor izquierdo", true, "atras"); break;
    case 11: mover(0, VEL);  resultado("Motor derecho", true, "adelante (verificar giro)"); break;
    case 12: mover(0, -VEL); resultado("Motor derecho", true, "atras"); break;
    case 13: ledcWrite(SUC, VEL_SUC); resultado("Succion", true, "encendida al 67 %"); break;
    default:
      Serial.printf("--- FIN: %d falla(s) ---\n\n", fallas);
      soloActuadores = false;
      cambiar(REPOSO, "fin de prueba", fallas ? "con fallas" : "todo OK");
      // 1 largo = OK, 3 cortos = fallas
      pitar(fallas ? 3 : 1, fallas ? 800 : 2000, fallas ? 150 : 400);
  }
}

// ================= menu por monitor serie =================
void menu(char c) {
  char txt[48];
  switch (c) {
    case 'h': case '?':
      Serial.println("s sensores | b bateria | e estado | m actuadores | t prueba");
      Serial.println("p paro | r rearme | l registro on/off | k simular cuelgue");
      break;
    case 's':
      Serial.printf("Distancia: frontal %s | lateral %s\n",
        distF >= SIN_ECO ? "sin eco" : (String(distF, 1) + " cm").c_str(),
        distL >= SIN_ECO ? "sin eco" : (String(distL, 1) + " cm").c_str());
      Serial.printf("Piso: %d cuentas | Bumper: %s | Paro: %s\n",
        adcPiso, digitalRead(BUMPER) ? "libre" : "presionado", paroActivo ? "ACTIVO" : "normal");
      break;
    case 'b': {
      float v = adcBat * K_BAT;
      int pct = constrain(map(adcBat, ADC_CORTE, 2743, 0, 100), 0, 100);
      Serial.printf("Bateria: %d cuentas | Vpin = %.2f V | Vbat = %.2f V | %d %%%s\n",
        adcBat, adcBat * 3.3 / 4095.0, v, pct, avisoBat ? " | AVISO" : "");
      break; }
    case 'e':
      Serial.printf("Estado: %s (hace %lu s) | vel %d | succion %s | log %s\n",
        NOMBRE[estado], (millis() - tEstado) / 1000, velAct,
        estado == ACTIVO ? "on" : "off", registro ? "on" : "off");
      break;
    case 'm': case 't':
      if (estado == REPOSO || estado == PAUSA) {
        soloActuadores = (c == 'm');
        cambiar(PRUEBA, "comando serie", c == 'm' ? "m" : "t");
      }
      else registrar("comando rechazado", "solo desde REPOSO o PAUSA");
      break;
    case 'p': cambiar(EMERGENCIA, "paro por software", "p"); break;
    case 'r':
      if (estado == EMERGENCIA && !paroActivo) cambiar(REPOSO, "rearme", "r");
      else registrar("rearme rechazado", paroActivo ? "paro fisico activo" : "no hay emergencia");
      break;
    case 'l':
      registro = !registro;
      Serial.printf("Registro %s\n", registro ? "activado" : "desactivado");
      break;
    // para probar el watchdog
    case 'k': registrar("cuelgue simulado", "loop detenido"); while (true) {}
  }
}

// ================= setup y loop =================
void setup() {
  Serial.begin(115200);
  pinMode(BOTON, INPUT_PULLUP); pinMode(BUMPER, INPUT_PULLUP); pinMode(PARO, INPUT_PULLUP);
  pinMode(TRIG_F, OUTPUT); pinMode(TRIG_L, OUTPUT); pinMode(ECHO_F, INPUT); pinMode(ECHO_L, INPUT);
  pinMode(LED_V, OUTPUT); pinMode(LED_R, OUTPUT);
  pinMode(IN1, OUTPUT); pinMode(IN2, OUTPUT); pinMode(IN3, OUTPUT); pinMode(IN4, OUTPUT);
  ledcAttach(ENA, 20000, 8); ledcAttach(ENB, 20000, 8);
  ledcAttach(SUC, 25000, 8);
  ledcAttach(ZUMB, 2000, 8); ledcWriteTone(ZUMB, 0);
  cortarTodo();
  attachInterrupt(digitalPinToInterrupt(BUMPER), isrBumper, FALLING);

  esp_task_wdt_config_t wdt = { .timeout_ms = 3000, .idle_core_mask = 0, .trigger_panic = true };
  // watchdog: si el loop se cuelga 3 s, la ESP32 se reinicia
  if (esp_task_wdt_init(&wdt) != ESP_OK) esp_task_wdt_reconfigure(&wdt);
  esp_task_wdt_add(NULL);

  Serial.println("\nRobot aspiradora - Taller 6. Enviar h para ver los comandos.");
  if (esp_reset_reason() == ESP_RST_TASK_WDT)
    registrar("reinicio", "por watchdog: actuadores apagados");
  else registrar("encendido", "listo");
  tEstado = millis();
}

void loop() {
  esp_task_wdt_reset();                 // "estoy vivo"
  leerSensores();
  int boton = leerBoton();
  char txt[40];

  // 1) parada de emergencia: maxima prioridad, desde cualquier estado
  if (paroActivo && estado != EMERGENCIA) cambiar(EMERGENCIA, "paro fisico", "GPIO 14");

  // 2) bateria con histeresis: segunda prioridad, el boton no la saca
  avisoBat = adcBat < ADC_AVISO;
  if (estado != EMERGENCIA && estado != ALERTA_BATERIA && adcBat < ADC_CORTE) {
    snprintf(txt, sizeof txt, "%d = %.2f V", adcBat, adcBat * K_BAT);
    cambiar(ALERTA_BATERIA, "bateria bajo corte", txt);
  }

  // 3) estados
  bool obstaculo = distF < D_OBST || adcPiso > UMBRAL_PISO || choque;
  switch (estado) {
    case REPOSO:
      if (boton == 1) cambiar(ACTIVO, "pulsacion corta", "boton");
      if (boton == 2) cambiar(PRUEBA, "pulsacion larga", "> 2 s");
      break;
    case ACTIVO:
      if (boton == 1) cambiar(PAUSA, "pulsacion corta", "boton");
      else if (boton == 2) cambiar(PRUEBA, "pulsacion larga", "> 2 s");
      else if (obstaculo) {
        if (choque) snprintf(txt, sizeof txt, "bumper");
        else if (adcPiso > UMBRAL_PISO) snprintf(txt, sizeof txt, "piso = %d", adcPiso);
        else snprintf(txt, sizeof txt, "frontal %.1f cm", distF);
        velAct = velObj = 0; mover(0, 0); tLibre = millis();
        cambiar(OBSTACULO, "obstaculo", txt);
      }
      break;
    case OBSTACULO:
      if (boton == 1) cambiar(PAUSA, "pulsacion corta", "boton");
      else if (boton == 2) cambiar(PRUEBA, "pulsacion larga", "> 2 s");
      else if (distF > D_LIBRE && adcPiso <= UMBRAL_PISO && digitalRead(BUMPER) == HIGH) {
        if (millis() - tLibre > 1000) { choque = false; cambiar(ACTIVO, "camino libre", "1 s"); }
      } else tLibre = millis();          // reinicia la espera mientras siga el obstaculo
      break;
    case PAUSA:
      if (boton == 1) cambiar(ACTIVO, "pulsacion corta", "boton");
      if (boton == 2) cambiar(PRUEBA, "pulsacion larga", "> 2 s");
      break;
    case ALERTA_BATERIA:
      if (adcBat > ADC_SALIDA) {
        snprintf(txt, sizeof txt, "%d cuentas", adcBat);
        cambiar(REPOSO, "bateria recuperada", txt);
      }
      break;
    case PRUEBA:
      if (boton == 1) { soloActuadores = false; cambiar(REPOSO, "prueba cancelada", "boton"); }
      else modoPrueba();
      break;
    case EMERGENCIA:
      if (boton == 2 && !paroActivo) cambiar(REPOSO, "rearme", "pulsacion larga");
      break;
  }
  if (estado != ACTIVO && estado != OBSTACULO) choque = false;

  // 4) actuadores segun el estado (la emergencia corta sin rampa)
  if (estado == ACTIVO) {
    velObj = VEL;
    static unsigned long tRampa = 0;
    if (millis() - tRampa >= 60) { tRampa = millis(); velAct = min(velAct + 15, velObj); }
    mover(velAct, velAct);
    ledcWrite(SUC, VEL_SUC);
  } else if (estado != PRUEBA) {
    cortarTodo();
  }

  // 5) interfaz
  if (Serial.available()) menu(Serial.read());
  if (estado != PRUEBA || paso > 3) senalizar();
  atenderZumbador();
}
