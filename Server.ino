#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
#include <WiFi.h>

// ==================== CONFIG WIFI ====================
const char* ssid = "SSID";
const char* password = "PASSWORD";

// ==================== CONFIG TCP =====================
WiFiServer server(80);

// ==================== CONFIG PCA9685 / SERVOS ==================
Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver(); // dirección por defecto 0x40

// Canales PCA9685 usados para los 5 servos (menique..pulgar)
const uint8_t CHANNELS[5] = { 0, 1, 2, 3, 4 };

// Pulso mínimo/máximo en ticks (0..4095) para tu servo a 50 Hz
// Ajusta SERVO_MIN y SERVO_MAX según tu servo concreto para que 0° y 70° coincidan físicamente
const int SERVO_MIN = 150; // pulso para 0° del servo (ticks)
const int SERVO_MAX = 600; // pulso para 180° del servo (ticks)

// Rango físico real que usaremos en grados
const int SERVO_PHYSICAL_MIN = 0;
const int SERVO_PHYSICAL_MAX = 70; // tu mano robótica llega a 70°

int angleToPulse(float angle_deg) {
  if (angle_deg < SERVO_PHYSICAL_MIN) angle_deg = SERVO_PHYSICAL_MIN;
  if (angle_deg > SERVO_PHYSICAL_MAX) angle_deg = SERVO_PHYSICAL_MAX;
  float fraction = (angle_deg - SERVO_PHYSICAL_MIN) / float(SERVO_PHYSICAL_MAX - SERVO_PHYSICAL_MIN);
  int pulse = SERVO_MIN + int(fraction * (SERVO_MAX - SERVO_MIN) + 0.5f);
  return pulse;
}

// ----------------- utilidades de parseo -----------------
// Espera formato "a0,a1,a2,a3,a4\n"
bool parse5(const String& s, float &a0, float &a1, float &a2, float &a3, float &a4) {
  int i1 = s.indexOf(',');
  if (i1 < 0) return false;
  int i2 = s.indexOf(',', i1 + 1);
  if (i2 < 0) return false;
  int i3 = s.indexOf(',', i2 + 1);
  if (i3 < 0) return false;
  int i4 = s.indexOf(',', i3 + 1);
  if (i4 < 0) return false;
  a0 = s.substring(0, i1).toFloat();
  a1 = s.substring(i1 + 1, i2).toFloat();
  a2 = s.substring(i2 + 1, i3).toFloat();
  a3 = s.substring(i3 + 1, i4).toFloat();
  a4 = s.substring(i4 + 1).toFloat();
  return true;
}

void setup() {
  Serial.begin(115200);
  delay(10);

  // Inicializar PCA9685
  Wire.begin();
  pwm.begin();
  pwm.setPWMFreq(50); // frecuencia típica para servos
  delay(10);

  // Conectar a WiFi
  Serial.print("Conectando a WiFi: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n✅ Conectado a WiFi!");
  Serial.print("📡 IP del ESP32: ");
  Serial.println(WiFi.localIP());

  server.begin();
  Serial.println("Servidor TCP escuchando en puerto 80...");

  // Posición inicial: todos en 0°
  int p_init = angleToPulse(0.0);
  for (int i = 0; i < 5; ++i) {
    pwm.setPWM(CHANNELS[i], 0, p_init);
  }
  Serial.println("Servos inicializados en 0 grados (pulso aplicado).");
}

void loop() {
  WiFiClient client = server.available();
  if (!client) {
    delay(1);
    return;
  }

  Serial.println("⚡ Cliente conectado");
  client.setTimeout(100); // evita bloqueos largos

  String line = "";
  unsigned long lastReceive = millis();

  while (client.connected()) {
    while (client.available()) {
      char c = client.read();
      lastReceive = millis();
      if (c == '\n') {
        line.trim();
        float a0, a1, a2, a3, a4;
        if (parse5(line, a0, a1, a2, a3, a4)) {
          // Recortar al rango físico
          float vals[5] = {a0, a1, a2, a3, a4};
          for (int i = 0; i < 5; ++i) {
            if (vals[i] < SERVO_PHYSICAL_MIN) vals[i] = SERVO_PHYSICAL_MIN;
            if (vals[i] > SERVO_PHYSICAL_MAX) vals[i] = SERVO_PHYSICAL_MAX;
          }

          // Convertir y aplicar a PCA9685
          for (int i = 0; i < 5; ++i) {
            int pulse = angleToPulse(vals[i]);
            pwm.setPWM(CHANNELS[i], 0, pulse);
            Serial.printf("Pin %d -> %.1f deg -> pulse %d\n", CHANNELS[i], vals[i], pulse);
          }

          Serial.print("➡ Recibido grados: ");
          Serial.printf("%.1f, %.1f, %.1f, %.1f, %.1f\n", a0, a1, a2, a3, a4);
        } else {
          Serial.print("❌ Formato inválido: ");
          Serial.println(line);
        }
        line = "";
      } else if (c != '\r') {
        line += c;
        if (line.length() > 128) line = ""; // protege buffer
      }
    }

    if (millis() - lastReceive > 2000) {
      break;
    }
    delay(1);
  }

  client.stop();
  Serial.println("🚪 Cliente desconectado");
}
