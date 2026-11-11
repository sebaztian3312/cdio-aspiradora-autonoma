# Robot aspiradora autónoma – Laboratorio Práctico CDIO I

Código del laboratorio práctico del proyecto CDIO I, Ingeniería Electrónica, Universidad del Quindío (2026).

**Equipo:** Mauricio Rodríguez Vargas, Andrés Felipe Reyes y Juan Sebastian Perez Galvis.

## Tarjeta y herramientas

- ESP32 DevKit con el paquete arduino-esp32 versión 3.x.
- Simulación en [Velxio](https://velxio.dev) (Talleres 1, 2, 3, 5 y 6) y en [Cirkit Designer](https://app.cirkitdesigner.com) (Taller 4).
- Cada programa está en su propia carpeta con el mismo nombre del `.ino`, como lo pide el IDE de Arduino.

## Contenido

| Carpeta | Contenido | Simulación |
|---|---|---|
| `Taller1_Entradas_digitales` | Pulsador y LED: pull-down, pull-up interna, LED 5 s, cambio de estado y antirrebote | — |
| `Taller2_Entradas_analogicas` | Potenciómetro, voltaje, resistencia R2, histéresis, promedio móvil, divisor de batería y porcentaje de carga | [Reto 1](https://velxio.dev/project/22675a26-ddee-4f5e-81c9-556762584178) · [2](https://velxio.dev/project/0ca64f87-e6c1-48df-be2b-96d3012785a6) · [3](https://velxio.dev/project/73a09b22-1201-41ab-91a0-e3cb762b04bf) · [4](https://velxio.dev/project/c2a36d09-8f36-4d6a-b83f-39b5276bc947) · [5](https://velxio.dev/project/0f2f259a-c5ad-4cf7-85db-32099ad8770c) · [6](https://velxio.dev/project/c88e8a67-ff73-4e2d-89b3-2084b7f081c1) |
| `Taller3_Salidas_PWM_actuadores` | Dimmer, motor con potenciómetro, configuración diferencial, rampa, frecuencia del PWM y succión | [Reto 2](https://velxio.dev/project/2dd4708f-7b8d-4792-ac2a-02b16223bffc) · [3](https://velxio.dev/project/50848fe4-10dd-4b04-b7ab-d946a2a10748) · [4](https://velxio.dev/project/9218c807-6a16-4384-a976-2f17461b5474) · [5](https://velxio.dev/project/54006867-757a-4e55-a37d-eb30ac00d035) |
| `Taller4_Sensores` | Bumper, HC-SR04 y sensor de piso | [Bumper](https://app.cirkitdesigner.com/project/5d24b828-90a0-4d2a-b4d0-ab6e3a631bcb) · [HC-SR04](https://app.cirkitdesigner.com/project/05940a7a-179c-484e-8e1e-835bad223ca5) · [Piso](https://app.cirkitdesigner.com/project/3bd50cd4-b147-40d9-a7a8-36dc858e1097) |
| `Taller5_Coordinacion` | Máquina de estados sin delay(): batería con histéresis, antirrebote, sensores, motores, bumper por interrupción y medición de tiempos | [Taller 5](https://velxio.dev/project/e506c6f5-4f9e-4106-8032-1bbc77461526) (también el archivo `Taller5_Coordinacion.vlx`) |
| `Taller6_Interfaz` | Sistema integrado: botón corto/largo, señalización, menú, modo prueba, registro y paro de emergencia | [Taller 6](https://velxio.dev/project/f16a9955-b4be-42c9-aa7f-abe75f81a379) |

## Plan de pines final

| Función | GPIO |
|---|---|
| Pulsador de inicio | 27 |
| LED verde / LED rojo | 26 / 13 |
| Batería (divisor 47 kΩ / 10 kΩ) | 34 |
| Motor izquierdo: ENA, IN1, IN2 | 19, 18, 17 |
| Motor derecho: ENB, IN3, IN4 | 16, 4, 21 |
| Motor de succión (MOSFET) | 25 |
| Bumper | 33 |
| Sensor de piso | 39 |
| Ultrasónico frontal: TRIG / ECHO | 32 / 35 |
| Ultrasónico lateral: TRIG / ECHO | 22 / 36 |
| Paro de emergencia | 14 |
| Zumbador | 23 |
