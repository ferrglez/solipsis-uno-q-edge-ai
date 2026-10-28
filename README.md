# Solipsis · Arduino UNO Q · Edge AI / Detección facial

Investigación experimental de detección facial con **RetinaFace MobileNet 0.25**, **ncnn** y **Vulkan/Mesa Turnip** sobre la GPU Qualcomm Adreno 702 del Arduino UNO Q.

## Estado comprobado
- Debian 13 ARM64, GPU Vulkan Turnip Adreno 702, OpenCL Rusticl FD702.
- ncnn compilado con Vulkan y RetinaFace funcional sobre imagen de prueba.
- Benchmark de 10 iteraciones (primera descartada): **CPU 68.868 ms** y **GPU 395.478 ms** de promedio.
- En esta configuración, GPU fue aproximadamente **5.74× más lenta** que CPU. Estos resultados no representan el rendimiento general del hardware.
- Pendiente: separar los tiempos de `create_extractor`, `ex.input`, `ex.extract` y posprocesamiento.

## Documentación
- [Entorno y hardware](docs/01-entorno-y-hardware.md)
- [Diagnóstico GPU](docs/02-diagnostico-gpu.md)
- [Dependencias](docs/03-dependencias.md)
- [Compilación ncnn](docs/04-compilacion-ncnn.md)
- [Validación Vulkan](docs/05-validacion-vulkan.md)
- [RetinaFace](docs/06-retinaface.md)
- [Benchmarks](docs/07-benchmarks.md)
- [Estado y siguientes pasos](docs/08-estado-y-siguientes-pasos.md)
- [Publicación GitHub](docs/09-publicacion-github.md)
- [Índice de comandos](logs/indice-comandos.md)

## Procedencia y límites
La documentación se preparó a partir de un registro de terminal de 5,687 líneas proporcionado por la persona operadora. La transcripción original se mantiene fuera del repositorio hasta terminar una revisión de información sensible. Los scripts son referencias auditables, no instaladores validados en una máquina limpia. Falta incorporar el archivo `retinaface.cpp` definitivo del Arduino para reproducir exactamente el benchmark.

**Importante:** detección de rostro no equivale a reconocimiento de identidad ni autenticación.
