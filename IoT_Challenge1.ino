#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// Dirección del LCD I2C
#define LCD_ADDRESS 0x27

// Definición de pines
#define ONE_WIRE_BUS A0   // Sensor de temperatura DS18B20
#define BUZZER_PIN 7      // Buzzer
#define TEMP_INCREMENTO_UMBRAL 5.0  // Incremento de temperatura en °C para alerta
#define TEMP_UMBRAL 27 
#define GAS_SENSOR_PIN A3
#define HUMIDITY_SENSOR_PIN A1  // Sensor de humedad
#define GAS_SENSOR_PIN A3       // Sensor de gas MQ-2
#define FLAME_SENSOR_PIN A2     // Sensor de llama
#define FLAME_DETECTED 500      // Valor de detección de llama
#define GAS_THRESHOLD 520       // Umbral del sensor de gas (ajustar según calibración)

// Inicialización de LCD y sensores
LiquidCrystal_I2C lcd(LCD_ADDRESS, 16, 2);
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

// Variables de temperatura
float tempBase = 0.0;
float tempAnterior = 0.0;
// Variables de color
int ledRojo = 9;
int ledVerde = 10;
int ledAzul = 11;
// Variable de estado del buzzer
bool buzzerActive;

// Función para detectar fuego considerando cada sensor por separado
bool checkFire(float tempC, int gasValue, int flameValue, String &reason) {
    bool fireDetected = false;
    reason = "";

    // Verificar incremento de temperatura con respecto a la temperatura base
    if (tempC - tempBase >= TEMP_INCREMENTO_UMBRAL) {
        Serial.println("ALERTA: Incremento rápido de temperatura!");
        fireDetected = true;
        reason = "Aum temp.";
    }
    tempAnterior = tempC;

    if (gasValue >= GAS_THRESHOLD) {
        Serial.println("ALERTA: Gas detectado!");
        fireDetected = true;
        reason = "Gas";
    }
    
    if (flameValue <= FLAME_DETECTED) {
        Serial.println("ALERTA: Llama detectada!");
        fireDetected = true;
        reason = "Fuego directo";
    }
    
    // Control de LEDs y buzzer según la alerta
    if (fireDetected) {
        analogWrite(ledRojo, 0);  // Rojo cuando hay incendio
        analogWrite(ledVerde, 255);   // Apagar verde
        analogWrite(ledAzul, 255);    // Apagar azul
        digitalWrite(BUZZER_PIN, LOW); // Activar buzzer
        buzzerActive = true;
    } else if (tempC - tempBase >= TEMP_INCREMENTO_UMBRAL - 3.0 || gasValue >= GAS_THRESHOLD * 0.9) { // Advertencia (naranja)
        analogWrite(ledRojo, 0);  // Naranja (rojo al 100%)
        analogWrite(ledVerde, 165); // Verde al 60%
        analogWrite(ledAzul, 255);    // Apagar azul
        digitalWrite(BUZZER_PIN, HIGH); // Buzzer apagado en advertencia
        buzzerActive = false;
    } else { // Estado normal (verde)
        analogWrite(ledRojo, 255);    // Apagar rojo
        analogWrite(ledVerde, 0); // Verde al 100%
        analogWrite(ledAzul, 255);    // Apagar azul
        digitalWrite(BUZZER_PIN, HIGH); // Apagar el buzzer
        buzzerActive = false;
    }
    
    return fireDetected;
}

void setup() {
    pinMode(ledRojo, OUTPUT);
    pinMode(ledVerde, OUTPUT);
    pinMode(ledAzul, OUTPUT);
    Serial.begin(9600);
    sensors.begin();
    lcd.init();
    lcd.backlight();  // Encender la luz de fondo del LCD

    pinMode(BUZZER_PIN, OUTPUT);
    pinMode(HUMIDITY_SENSOR_PIN, INPUT);
    pinMode(GAS_SENSOR_PIN, INPUT);
    pinMode(FLAME_SENSOR_PIN, INPUT);

    digitalWrite(BUZZER_PIN, LOW); // Asegurar que el buzzer comienza apagado

    // Leer temperatura base después de estabilizar el sensor
    delay(500);
    sensors.requestTemperatures();
    tempBase = sensors.getTempCByIndex(0);
    tempAnterior = tempBase;
    Serial.print("Temperatura base registrada: ");
    Serial.print(tempBase);
    Serial.println(" °C");
}

void loop() {
    sensors.requestTemperatures();
    float tempC = sensors.getTempCByIndex(0);
    int gasValue = analogRead(GAS_SENSOR_PIN);
    int humidityValue = analogRead(HUMIDITY_SENSOR_PIN);
    int flameValue = analogRead(FLAME_SENSOR_PIN);
    String fireReason = "Ninguna";

    // Evaluar si se detecta fuego
    bool fireDetected = checkFire(tempC, gasValue, flameValue, fireReason);
    
    // Mostrar en Monitor Serie
    Serial.print("Temp: "); Serial.print(tempC); Serial.println(" °C");
    Serial.print("Gas: "); Serial.print(gasValue);
    Serial.print("Flame: "); Serial.print(flameValue <= FLAME_DETECTED ? "Llama detectada!" : "No hay llama.");
    
    // Mostrar en LCD
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Temp: "); 
    lcd.print(tempC);
    lcd.print("C");

    lcd.setCursor(0, 1);
    if (fireDetected) {
        lcd.print("ALERTA: "); 
        lcd.print(fireReason);
    } else {
        lcd.print("Gas:");
        lcd.print(gasValue);
        lcd.print(" Hum:");
        lcd.print(humidityValue);
    }

    delay(500); // Espera antes de la siguiente lectura
}
