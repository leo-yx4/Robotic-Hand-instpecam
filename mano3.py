import cv2
import mediapipe as mp
import numpy as np
from math import acos, degrees
import socket
import time

ESP32_IP = '192.168.137.61'
PORT = 80
THRESH_EXTENDED = 60.0  # umbral para considerar dedo extendido

# Puntos de referencia para cada dedo
finger_points = {
    "pulgar": [1, 2, 4],
    "indice": [5, 6, 8],
    "medio": [9, 10, 12],
    "anular": [13, 14, 16],
    "menique": [17, 18, 20]
}

# Mapeo dedo → pin PCA9685
finger_to_pin = {
    "menique": 0,
    "anular": 1,
    "medio": 2,
    "indice": 3,
    "pulgar": 4
}

# Inicialización de MediaPipe
mp_drawing = mp.solutions.drawing_utils
mp_drawing_styles = mp.solutions.drawing_styles
mp_hands = mp.solutions.hands

# Función para calcular ángulo entre tres puntos
def calc_angle(p1, p2, p3):
    l1 = np.linalg.norm(p2 - p3)
    l2 = np.linalg.norm(p1 - p3)
    l3 = np.linalg.norm(p1 - p2)
    if l1 != 0 and l3 != 0:
        cos_val = (l1**2 + l3**2 - l2**2) / (2 * l1 * l3)
        cos_val = max(-1.0, min(1.0, cos_val))
        return degrees(acos(cos_val))
    return 0.0

# Función para conectar al ESP32
def conectar_socket():
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.connect((ESP32_IP, PORT))
        print("✅ Conectado al ESP32")
        return s
    except Exception as e:
        print("❌ Error al conectar:", e)
        return None

# Abrir cámara
cap = cv2.VideoCapture(1, cv2.CAP_DSHOW)
if not cap.isOpened():
    print("❌ No se pudo abrir la cámara")
    exit()

# Conectar al ESP32
s = conectar_socket()

with mp_hands.Hands(
    model_complexity=1,
    max_num_hands=1,
    min_tracking_confidence=0.5) as hands:

    while True:
        ret, frame = cap.read()
        if not ret:
            print("⚠️ No se pudo leer el frame")
            continue

        frame = cv2.flip(frame, 1)
        h, w, _ = frame.shape
        frame_rgb = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
        results = hands.process(frame_rgb)

        # Inicializar ángulos por pin
        pca_angles = [0, 0, 0, 0, 0]

        if results.multi_hand_landmarks:
            for hand_landmarks in results.multi_hand_landmarks:
                angulos = {}

                for dedo, puntos in finger_points.items():
                    p1 = hand_landmarks.landmark[puntos[0]]
                    p2 = hand_landmarks.landmark[puntos[1]]
                    p3 = hand_landmarks.landmark[puntos[2]]

                    p1 = np.array([p1.x * w, p1.y * h])
                    p2 = np.array([p2.x * w, p2.y * h])
                    p3 = np.array([p3.x * w, p3.y * h])

                    angle = calc_angle(p1, p2, p3)
                    angulos[dedo] = angle

                # Mostrar ángulos en pantalla
                x_base = w - 220
                y_base = 40
                line_height = 22
                for i, dedo in enumerate(["pulgar", "indice", "medio", "anular", "menique"]):
                    if dedo in angulos:
                        texto = f"{dedo}: {angulos[dedo]:.1f}"
                        posicion = (x_base, y_base + i * line_height)
                        cv2.putText(frame, texto, posicion,
                                    cv2.FONT_HERSHEY_SIMPLEX, 0.6, (0, 0, 0), 2)

                # Decidir ángulos discretos por dedo
                for dedo, angle in angulos.items():
                    pin = finger_to_pin[dedo]
                    is_extended = angle > THRESH_EXTENDED

                    if dedo == "indice":
                        servo_angle = 70 if is_extended else 0
                    else:
                        servo_angle = 0 if is_extended else 70

                    pca_angles[pin] = servo_angle

                # Enviar al ESP32
                mensaje = "{},{},{},{},{}\n".format(*pca_angles)
                print(f"➡ Enviando: {mensaje.strip()}")

                try:
                    if s:
                        s.sendall(mensaje.encode())
                except Exception as e:
                    print("⚠️ Error al enviar:", e)
                    if s:
                        s.close()
                    s = conectar_socket()
                    if not s:
                        print("❌ No se pudo reconectar. Saliendo...")
                        break

                # Dibujar mano
                mp_drawing.draw_landmarks(
                    frame,
                    hand_landmarks,
                    mp_hands.HAND_CONNECTIONS,
                    mp_drawing_styles.get_default_hand_landmarks_style(),
                    mp_drawing_styles.get_default_hand_connections_style())

        cv2.imshow("Frame", frame)
        if cv2.waitKey(1) & 0xFF == 27:
            break

    if s:
        s.close()

cap.release()
cv2.destroyAllWindows()
