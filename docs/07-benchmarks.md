# Benchmark exploratorio de RetinaFace

Modelo: RetinaFace MobileNet 0.25 con ncnn. Imagen: `test.jpg` (512 × 512). Se ejecutaron 10 iteraciones por modo; se descartó la primera como calentamiento.

| Iteración | CPU (ms) | GPU Vulkan (ms) |
|---:|---:|---:|
| 1 | 84.866 | 448.686 |
| 2 | 78.245 | 395.754 |
| 3 | 68.297 | 395.518 |
| 4 | 72.175 | 394.836 |
| 5 | 67.323 | 395.036 |
| 6 | 67.477 | 395.418 |
| 7 | 66.917 | 395.781 |
| 8 | 66.456 | 395.639 |
| 9 | 66.514 | 395.479 |
| 10 | 66.409 | 395.843 |

**Media de iteraciones 2–10:** CPU **68.868 ms**, GPU **395.478 ms**. Relación GPU/CPU ≈ **5.74×**. El tiempo incluye etapas del detector, no solamente ejecución de kernels GPU; no debe interpretarse como FPS de cámara ni una comparación definitiva de hardware.

También se realizaron cinco ejecuciones individuales: CPU 81.243, 84.312, 84.098, 85.783, 91.780 ms (media 85.44 ms); GPU 478.125, 446.192, 448.776, 476.834, 448.484 ms (media 459.68 ms). No mezclar estas ejecuciones con el benchmark de diez iteraciones.
