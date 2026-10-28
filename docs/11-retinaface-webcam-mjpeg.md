# RetinaFace en vivo: webcam, Vulkan y MJPEG

Fecha: 28 de octubre de 2026

## 1. Objetivo

Implementar detección facial continua en el Arduino UNO Q,
utilizando RetinaFace MobileNet 0.25 con ncnn y Vulkan.

La imagen procesada se transmite mediante un servidor MJPEG
para visualizar las detecciones desde un navegador web.

## 2. Hardware y software

| Componente | Configuración |
|---|---|
| Plataforma | Arduino UNO Q |
| Sistema operativo | Debian 13 ARM64 |
| GPU | Qualcomm Adreno 702 |
| Controlador Vulkan | Mesa Turnip |
| Cámara | Logitech C920 |
| Captura | 640 x 480, MJPEG |
| Modelo | RetinaFace MobileNet 0.25 |
| Motor de inferencia | ncnn |
| Aceleración | Vulkan |
| Servidor web | Python HTTP + MJPEG |
| Puerto | 8080 |

## 3. Arquitectura

Logitech C920
    |
    v
OpenCV VideoCapture
    |
    v
RetinaFace + ncnn + Vulkan
    |
    v
Detección de rostros y landmarks
    |
    v
OpenCV: dibujo de resultados
    |
    v
retinaface_latest.jpg
    |
    v
Servidor HTTP/MJPEG
    |
    v
Navegador web

El modelo se carga una sola vez al iniciar el ejecutable.

## 4. Archivos del repositorio

- `src/retinaface_webcam.cpp`: captura e inferencia continua.
- `web/stream_server.py`: servidor HTTP/MJPEG.
- `src/retinaface-fp16/`: experimentos anteriores.
- `benchmarks/`: resultados de rendimiento.
- `docs/`: documentación de instalación y pruebas.

## 5. Dependencias

Se requiere:

- ncnn compilado con NCNN_VULKAN=ON.
- OpenCV con soporte de VideoCapture y JPEG.
- Vulkan operativo sobre Adreno 702.
- Python 3.
- Modelo RetinaFace en formato ncnn (.param y .bin).

Consultar la documentación anterior del repositorio
para preparar el entorno y descargar el modelo.

## 6. Compilación

En el entorno de desarrollo utilizado:

    cd ~/solipsis-edge-ai/ncnn

Copiar el archivo del repositorio:

    cp ~/solipsis-uno-q-edge-ai/src/retinaface_webcam.cpp \
       examples/retinaface_webcam.cpp

Registrar el ejecutable en examples/CMakeLists.txt,
junto a los demás ejemplos de RetinaFace:

    ncnn_add_example(retinaface_webcam)

Configurar y compilar:

    cmake -S . -B build
    cmake --build build --target retinaface_webcam -j2

Nota: el archivo C++ contiene rutas absolutas al modelo
y al directorio de imágenes utilizadas en este prototipo.

## 7. Ejecución

Terminal 1: iniciar RetinaFace.

    cd ~/solipsis-edge-ai/ncnn
    ./build/examples/retinaface_webcam

Terminal 2: iniciar el servidor MJPEG.

    cd ~/solipsis-uno-q-edge-ai
    python3 web/stream_server.py

Abrir en el navegador:

    http://IP_DEL_UNO_Q:8080

La computadora y el UNO Q deben tener conectividad de red.

## 8. Resultados experimentales

En las pruebas del 28 de octubre de 2026:

- Se confirmó el uso de Vulkan con Adreno 702.
- La webcam funcionó a 640 x 480.
- La inferencia se mantuvo aproximadamente entre
  475 y 513 ms por fotograma, después del arranque.
- El rendimiento de inferencia fue cercano a 2 FPS.
- Se observaron hasta seis rostros detectados
  simultáneamente durante una prueba visual.
- Se visualizaron recuadros, puntos faciales y
  puntuaciones de confianza en el navegador.
- El procesamiento continuó durante más de 90 fotogramas.

Los FPS indicados corresponden aproximadamente a la
inferencia y no representan una medición formal de
rendimiento extremo a extremo de la transmisión MJPEG.

## 9. Limitaciones

- Rendimiento cercano a 2 FPS.
- Rutas absolutas en el código C++.
- Dispositivo de captura configurado como /dev/video2.
- El servidor MJPEG no incorpora autenticación.
- La transmisión HTTP no está cifrada.
- El archivo JPEG se actualiza en disco continuamente.

El servidor está destinado a pruebas en una red local
de confianza, no a exposición pública en Internet.

## 10. Próximos pasos

1. Comparar el rendimiento continuo entre CPU y GPU.
2. Optimizar la resolución de entrada.
3. Medir latencia extremo a extremo.
4. Reducir el costo de transferencia y codificación JPEG.
5. Parametrizar cámara, modelo y puerto.
6. Integrar el detector en la arquitectura Edge AI.
