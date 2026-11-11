// Taller 2 - Reto 4: 20 lecturas con la perilla quieta, estadisticas y promedio movil
const int POT = 34;
const int N = 20;         // lecturas por prueba
const int VENTANA = 8;    // tamano del promedio movil

int historial[VENTANA];
int indice = 0;
long suma = 0;

float promedioMovil(int nueva) {
  suma -= historial[indice];     // sale la lectura mas vieja
  historial[indice] = nueva;     // entra la nueva
  suma += nueva;
  indice = (indice + 1) % VENTANA;
  return (float)suma / VENTANA;
}

void estadisticas(const float* d, int n, float &media, float &desv, float &rango) {
  float s = 0, mn = d[0], mx = d[0];
  for (int i = 0; i < n; i++) {
    s += d[i];
    if (d[i] < mn) mn = d[i];
    if (d[i] > mx) mx = d[i];
  }
  media = s / n;
  float s2 = 0;
  for (int i = 0; i < n; i++) s2 += (d[i] - media) * (d[i] - media);
  desv = sqrt(s2 / (n - 1));     // desviacion estandar muestral
  rango = mx - mn;
}

void setup() {
  Serial.begin(115200);
  int primera = analogRead(POT);           // llena el historial para arrancar estable
  for (int i = 0; i < VENTANA; i++) historial[i] = primera;
  suma = (long)primera * VENTANA;
}

void loop() {
  float crudas[N], filtradas[N];
  for (int i = 0; i < N; i++) {
    // se agrega ruido porque el potenciometro simulado no lo tiene
    int lectura = analogRead(POT) + random(-20, 21);
    crudas[i] = lectura;
    filtradas[i] = promedioMovil(lectura);
    delay(10);
  }

  float mC, dC, rC, mF, dF, rF;
  estadisticas(crudas, N, mC, dC, rC);
  estadisticas(filtradas, N, mF, dF, rF);

  Serial.println("n  | cruda | filtrada");
  for (int i = 0; i < N; i++) {
    Serial.printf("%2d | %5.0f | %8.1f\n", i + 1, crudas[i], filtradas[i]);
  }
  Serial.printf("Cruda:    promedio = %.1f  desviacion = %.2f  rango = %.0f\n", mC, dC, rC);
  Serial.printf("Filtrada: promedio = %.1f  desviacion = %.2f  rango = %.1f\n", mF, dF, rF);
  Serial.println("----------------------------------------");
  delay(5000);   // una prueba cada 5 s
}
