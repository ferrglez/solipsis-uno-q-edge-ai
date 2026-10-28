# Solipsis · Arduino UNO Q · Edge AI / Detección facial

Investigación experimental de detección facial con **RetinaFace MobileNet 0.25**, **ncnn** y **Vulkan/Mesa Turnip** sobre la GPU Qualcomm Adreno 702 del Arduino UNO Q.

## Actualización experimental — 28 de octubre de 2026

Se completaron profiling por etapas y por 90 capas Vulkan y cuatro configuraciones FP16. En las mediciones reportadas, la configuración (packed=0, storage=0, arithmetic=0) promedió **388.415 ms** frente a **401.232 ms** de (1,1,1) (tres rondas); (1,1,0) registró **420.273 ms** y (0,0,1) **388.268 ms** (una ejecución cada una). Estos resultados son preliminares y aún falta repetir las cuatro variantes en orden alternado. **Los archivos fuente experimentales y logs residen en el Arduino y todavía no se han sincronizado con GitHub.**

Consulta [la bitácora técnica paso a paso](docs/10-profiling-vulkan-fp16-2026-10-28.md).

## Estado comprobado
- Debian 13 ARM64, GPU Vulkan Turnip Adreno 702, OpenCL Rusticl FD702.
- ncnn compilado con Vulkan y RetinaFace funcional sobre imagen de prueba.
- Benchmark de 10 iteraciones (primera descartada): **CPU 68.868 ms** y **GPU 395.478 ms** de promedio.
- En esta configuración, GPU fue aproximadamente **5.74× más lenta** que CPU. Estos resultados no representan el rendimiento general del hardware.
- Posteriormente se instrumentaron las extracciones y el posprocesamiento, además del profiling interno Vulkan; ver bitácora de octubre. Aún falta cuantificar por separado transferencias y sincronización GPU→CPU.

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
- [Profiling Vulkan y experimentos FP16 (28-oct-2026)](docs/10-profiling-vulkan-fp16-2026-10-28.md)

## Procedencia y límites
La documentación se preparó a partir de un registro de terminal de 5,687 líneas proporcionado por la persona operadora. La transcripción original se mantiene fuera del repositorio hasta terminar una revisión de información sensible. Los scripts son referencias auditables, no instaladores validados en una máquina limpia. El repositorio incluye una copia anterior de `src/retinaface.cpp`, pero faltan las versiones instrumentadas recientes y sus logs para reproducir exactamente los experimentos actuales.

**Importante:** detección de rostro no equivale a reconocimiento de identidad ni autenticación.
