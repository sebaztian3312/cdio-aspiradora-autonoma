// Taller 2 - Reto 6: porcentaje de carga y umbrales provisionales
// Divisor R1 = 47 kOhm / R2 = 10 kOhm al GPIO 34. Bateria supuesta: litio 3S (provisional)

const int PIN_BAT = 34;
const float R1 = 47000.0;
const float R2 = 10000.0;
const float FACTOR = (R1 + R2) / R2;

// Umbrales provisionales en cuentas del ADC (se reutilizan en el Taller 5)
const int ADC_LLENA = 2743;   // 12,6 V -> 100 %
const int ADC_AVISO = 2286;   // 10,5 V -> aviso de bateria baja
const int ADC_CORTE = 2155;   //  9,9 V -> corte, 0 %

const int VENTANA = 8;
int historial[VENTANA];
int indice = 0;
long suma = 0;

float promedioMovil(int nueva) {
  suma -= historial[indice];
  historial[indice] = nueva;
  suma += nueva;
  indice = (indice + 1) % VENTANA;
  return (float)suma / VENTANA;
}

float porcentajeCarga(float cuentas) {
  float p = (cuentas - ADC_CORTE) * 100.0 / (ADC_LLENA - ADC_CORTE);
  return constrain(p, 0.0, 100.0);   // limita a 0-100 %
}

void setup() {
  Serial.begin(115200);
  int primera = analogRead(PIN_BAT);
  for (int i = 0; i < VENTANA; i++) historial[i] = primera;
  suma = (long)primera * VENTANA;
}

void loop() {
  float cuentas = promedioMovil(analogRead(PIN_BAT));
  float vBat = cuentas * 3.3 / 4095.0 * FACTOR;
  float carga = porcentajeCarga(cuentas);

  const char* estado;
  if (cuentas <= ADC_CORTE)      estado = "CORTE";
  else if (cuentas <= ADC_AVISO) estado = "AVISO";
  else                           estado = "NORMAL";

  Serial.printf("ADC = %4.0f | Vbat = %.2f V | Carga = %3.0f %% | %s\n",
                cuentas, vBat, carga, estado);
  delay(500);
}
