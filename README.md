# Mano Robótica con Visión por Computador y ESP32

## Descripción del proyecto
Este sistema permite controlar una mano robótica de 5 dedos en tiempo real usando la cámara de un portátil. Con un script Python (`mano3.py`) se capturan gestos de la mano humana usando **MediaPipe** y **OpenCV**, obteniendo 21 puntos clave (“landmarks”) de la mano. A partir de esos puntos se calculan ángulos (por ejemplo, ley de cosenos para el pulgar) y distancias para determinar qué dedos están extendidos. Luego el script envía, vía sockets TCP, los ángulos de los servos al ESP32.  
En el ESP32 corre el sketch Arduino `LISTENING2.ino`, que monta un servidor WiFi (usando `WiFiServer`) para recibir los comandos y los convierte en pulsos PWM mediante el controlador PCA9685, moviendo los servomotores SG90 de la mano robótica.

---

## 🧰 Materiales necesarios

### Hardware
- ESP32 (por ejemplo, ESP32 DevKit)
- Controlador de servos PCA9685 (16 canales, I2C)
- 5 servomotores SG90 (5V, ~180°)
- Fuente de alimentación 5V 5A
- Cableado I2C y de alimentación
- Cámara del portátil o webcam USB

### Software
- Python 3.12 o superior
- Visual Studio Code
- Arduino IDE
- Librerías Python: `mediapipe`, `opencv-python`, `numpy`
- Librería Arduino: `Adafruit PWM Servo Driver` (PCA9685)

---

## ⚙️ Instalación de dependencias

### En Python
```bash
pip install mediapipe opencv-python numpy
```
(MediaPipe 0.10.x soporta Python 3.9–3.12)

### En Arduino IDE
1. Abrir **Sketch → Include Library → Manage Libraries**
2. Buscar **Adafruit PWM Servo Driver**
3. Instalar la librería

---

## 🔌 Conexiones eléctricas

### PCA9685 ↔ ESP32
| PCA9685 | ESP32 |
|----------|-------|
| SDA | GPIO21 |
| SCL | GPIO22 |
| VCC | 3.3V |
| GND | GND |

**Importante:** unir todas las tierras (GND común).

### Alimentación de servos
- Fuente 5V 5A → terminales V+ y GND del PCA9685.  
- No alimentar servos desde el pin 5V del ESP32.  
- El PCA9685 distribuye la alimentación a los servos.

### Servos SG90
- Conectar cada servo a un canal PWM del PCA9685 (canales 0–4).  
- Señal → pin PWM, rojo → V+, negro → GND.  
- Configurar frecuencia: `pwm.setPWMFreq(60)`.

---

## ▶️ Ejecución de los programas

### En el ESP32
1. Abrir `LISTENING2.ino` en Arduino IDE.
2. Configurar red WiFi (SSID y contraseña).
3. Subir el código.
4. Verificar IP en el monitor serie.

Ejemplo:
```cpp
const uint ServerPort = 23;
WiFiServer server(ServerPort);
void setup() {
  WiFi.begin(ssid, password);
  server.begin();
  pwm.begin();
  pwm.setPWMFreq(60);
}
```

### En Python
Editar IP y puerto en `mano3.py`:
```python
ESP32_IP = '192.168.137.61'
PORT = 80
```
Ejecutar:
```bash
python mano3.py
```

---

## ✋ Interpretación de gestos

El script detecta si cada dedo está extendido o flexionado.  
Se genera un vector `[pulgar, índice, medio, anular, meñique]` con valores `True` (extendido) o `False` (flexionado).

| Gesto | Vector | Acción |
|-------|---------|---------|
| Mano abierta | [T, T, T, T, T] | Abrir mano |
| Puño cerrado | [F, F, F, F, F] | Cerrar mano |
| Seña V | [F, T, T, F, F] | Extender índice y medio |
| Pulgar arriba | [T, F, F, F, F] | Gesto de aprobación |

---

## 🧩 Solución de problemas

- **MediaPipe no se instala:** use Python 64-bit (3.9–3.12).  
- **Cámara no abre:** use `cv2.VideoCapture(0, cv2.CAP_DSHOW)`.  
- **ESP32 no conecta:** revise SSID y contraseña.  
- **Servos tiemblan:** fuente de 5V inestable o sin tierra común.  
- **Python no encuentra el ESP32:** ambos deben estar en la misma red.  
- **Error “Connection refused”:** revisar puertos en Python y Arduino.  
- **Código Arduino no compila:** verificar librería `Adafruit_PWMServoDriver` instalada.  

---

## 📁 Estructura recomendada del proyecto

```
mano-robotica/
├─ arduino/
│  └─ LISTENING2.ino
├─ python/
│  └─ mano3.py
├─ docs/
│  └─ diagrama_conexiones.png
├─ README.md
└─ .gitignore
```

---

## 🚀 Subir el proyecto a GitHub

```bash
git init
git add .
git commit -m "Primer commit: Mano robótica con cámara y ESP32"
git branch -M main
git remote add origin https://github.com/USUARIO/mano-robotica.git
git push -u origin main
```

### `.gitignore` recomendado
```
__pycache__/
*.pyc
env/
.vscode/
build/
*.bin
*.elf
```

---

## 🪪 Licencia

Este proyecto está bajo la licencia **MIT**.
