# RetinaFace en Arduino UNO Q — profiling y experimentos FP16

**Fecha:** 28 de octubre de 2026  
**Estado:** experimentos ejecutados en dispositivo; sincronización de código y logs locales pendiente de verificar.

## 1. Objetivo y plataforma

Investigar por qué RetinaFace MobileNet 0.25 con ncnn Vulkan tiene mayor latencia que CPU en Arduino UNO Q, e identificar cuellos de botella y opciones de precisión que mejoran o empeoran el rendimiento.

- Plataforma: Arduino UNO Q, Debian 13 ARM64, GPU Qualcomm Adreno 702, Vulkan mediante Mesa Turnip 25.2.6.
- ncnn: versión observada en CMake `1.0.20261028`, compilado con `NCNN_VULKAN=ON`, `NCNN_BENCHMARK=ON`, `NCNN_BUILD_EXAMPLES=ON`.
- OpenCV 4.10.0.
- Modelo: `mnet.25-opt.param` / `mnet.25-opt.bin`; imagen `test.jpg` 512×512.
- Directorio de trabajo: `~/solipsis-edge-ai/ncnn`. Ejecutables: `ncnn/build/examples/`; modelo e imagen: `~/solipsis-edge-ai/models/retinaface/`.

**Precaución:** las opciones de ncnn expresan preferencias de ejecución; no demuestran por sí mismas la precisión real de cada shader/capa. Los FPS son inversos aproximados de la latencia medida, no rendimiento de una cámara en tiempo real.

## 2. Cronología paso a paso

### A. Establecer una línea base CPU/GPU

1. Compilar ncnn con Vulkan y el ejemplo RetinaFace.
2. Ejecutar el detector sobre la misma imagen en CPU y GPU.
3. Registrar iteraciones, descartando calentamiento según el experimento.
4. La documentación histórica del repositorio registra **68.868 ms CPU / 395.478 ms GPU** en una medición inicial. Posteriormente, el ejemplo instrumentado produjo **69.868 ms CPU / 396.961 ms GPU**. Son ejecuciones distintas y no deben mezclarse.
5. Hipótesis inicial: cada una de las nueve extracciones de salida podía introducir descargas GPU→CPU y sincronización.

### B. Instrumentar la ejecución

6. Crear `examples/retinaface_profiling.cpp` a partir del ejemplo y medir las nueve llamadas `ex.extract` (tres salidas para cada stride 32, 16 y 8), propuestas, ordenación y NMS.
7. Resultados de extracción aproximados:

| Etapa | CPU (ms) | GPU (ms) |
|---|---:|---:|
| Stride 32 | 38.39 | 253.66 |
| Stride 16 | 6.82 | 34.01 |
| Stride 8 | 23.78 | 108.04 |

8. El posprocesamiento fue menor a 0.4 ms en estas mediciones. Esto **no** cuantifica aisladamente el costo de descargas/sincronización; solo muestra dónde se observa el tiempo total.

### C. Activar profiling interno de capas Vulkan

9. Configurar CMake con `-DNCNN_BENCHMARK=ON` y recompilar. Durante una reconstrucción aparecieron objetos vacíos `mat.cpp.o` y `layer.cpp.o`; se eliminaron esos dos archivos y se recompiló el objetivo con `-j2`.
10. El registro `retinaface_vulkan_layers.log` reportó 90 capas por iteración, latencia total GPU **400.906 ms** y suma aproximada de tiempos por capa **371.291 ms**.
11. Desglose aproximado por extracción: stride 32 **244.018 ms**, stride 16 **27.760 ms**, stride 8 **99.271 ms**.
12. Entre las capas más caras estuvo `rf_c1_aggr`, aproximadamente **30.484 ms**.

### D. Identificar opciones FP16 y construir variantes

13. Inspeccionar `ncnn/src/option.cpp`: las opciones `use_fp16_packed`, `use_fp16_storage`, `use_fp16_arithmetic` están habilitadas por defecto.
14. Agregar diagnóstico antes de cargar el modelo en el ejemplo instrumentado:

```cpp
fprintf(stderr, "NCNN_OPTIONS vulkan=%d fp16_packed=%d fp16_storage=%d fp16_arithmetic=%d\n",
        (int)retinaface.opt.use_vulkan_compute,
        (int)retinaface.opt.use_fp16_packed,
        (int)retinaface.opt.use_fp16_storage,
        (int)retinaface.opt.use_fp16_arithmetic);
```

15. Crear cuatro ejecutables a partir de `retinaface_profiling.cpp`, insertando las opciones inmediatamente después de `retinaface.opt.use_vulkan_compute = use_gpu;` y antes del diagnóstico:

| Archivo fuente en `ncnn/examples/` | Packed | Storage | Arithmetic |
|---|---:|---:|---:|
| `retinaface_profiling.cpp` | 1 | 1 | 1 |
| `retinaface_fp32.cpp` | 0 | 0 | 0 |
| `retinaface_no_fp16_arithmetic.cpp` | 1 | 1 | 0 |
| `retinaface_fp16_arithmetic_only.cpp` | 0 | 0 | 1 |

16. Registrar en `ncnn/examples/CMakeLists.txt` cada nuevo objetivo mediante `ncnn_add_example(nombre_del_objetivo)`.
17. Reconfigurar y compilar cada objetivo. Los dos últimos se compilaron correctamente con `EXIT CODE: 0`.

### E. Ejecutar benchmarks y comparar

18. Ejecutar cada binario desde `~/solipsis-edge-ai/models/retinaface` con argumentos `test.jpg gpu`, redirigiendo stdout/stderr a logs.
19. En la primera comparación de tres rondas alternadas (FP16 completo primero, luego FP16 desactivado), las latencias fueron:

| Ronda | (1,1,1) ms | (0,0,0) ms |
|---|---:|---:|
| 1 | 401.323 | 388.325 |
| 2 | 401.165 | 388.453 |
| 3 | 401.207 | 388.469 |
| **Media** | **401.232** | **388.415** |

20. Desactivar las tres opciones mejoró **12.817 ms (~3.19%)** respecto de FP16 completo en esas rondas. El orden de ejecución podría influir; no se consideró prueba definitiva.
21. Se ejecutó `retinaface_no_fp16_arithmetic` con diagnóstico `vulkan=1 fp16_packed=1 fp16_storage=1 fp16_arithmetic=0`: **420.273 ms** (una ejecución), confianza **0.98340**.
22. Se ejecutó `retinaface_fp16_arithmetic_only` con diagnóstico `vulkan=1 fp16_packed=0 fp16_storage=0 fp16_arithmetic=1`: **388.268 ms** (una ejecución), confianza **0.98351**.

| Configuración (packed,storage,arithmetic) | Latencia GPU (ms) | Evidencia |
|---|---:|---|
| (1,1,1) | 401.232 | Media de 3 rondas |
| (0,0,0) | 388.415 | Media de 3 rondas |
| (1,1,0) | 420.273 | Una ejecución |
| (0,0,1) | 388.268 | Una ejecución |

23. En profiling interno, suma de 90 capas: **373.493 ms** para (1,1,1), **360.614 ms** para (0,0,0), **390.266 ms** para (1,1,0). Los totales de capas no son equivalentes a la latencia de extremo a extremo.
24. En la configuración (1,1,0), las capas con mayores incrementos frente a (1,1,1) incluyeron `mobilenet0_conv0_fwd` **+5.616 ms**, `rf_c1_aggr` **+3.061 ms**, `rf_c1_det_context_conv1` **+2.161 ms**, `rf_c1_det_conv1` **+2.144 ms**, `mobilenet0_conv4_fwd` **+2.076 ms**.

## 3. Interpretación prudente

- **Observación:** (0,0,0) y (0,0,1) fueron similares (~388 ms); (1,1,0) fue la más lenta (~420 ms).
- **Hipótesis:** la combinación de opciones de almacenamiento/empaquetado FP16 está asociada con una diferencia importante de rendimiento en este modelo/dispositivo.
- **No demostrado:** qué shader exacto se seleccionó, la precisión real por capa, ni el costo aislado de transferencias GPU→CPU.
- **No aislado todavía:** packed y storage se cambiaron juntos; sus contribuciones individuales siguen sin medirse.
- La diferencia de 0.147 ms entre (0,0,0) y (0,0,1) es demasiado pequeña para concluir que una sea más rápida sin repeticiones comparables.

## 4. Reproducir en el Arduino

```bash
cd ~/solipsis-edge-ai/ncnn
cmake -S . -B build -DNCNN_VULKAN=ON -DNCNN_BENCHMARK=ON -DNCNN_BUILD_EXAMPLES=ON
cmake --build build --target retinaface_profiling retinaface_fp32 retinaface_no_fp16_arithmetic retinaface_fp16_arithmetic_only -j2
cd ~/solipsis-edge-ai/models/retinaface
~/solipsis-edge-ai/ncnn/build/examples/retinaface_fp16_arithmetic_only test.jpg gpu > ~/solipsis-edge-ai/retinaface_fp16_arithmetic_only.log 2>&1
grep -E 'NCNN_OPTIONS|AVERAGE GPU' ~/solipsis-edge-ai/retinaface_fp16_arithmetic_only.log
```

El comando de compilación presupone que **los cuatro archivos fuente y sus registros CMake ya están presentes**. Los modelos y la imagen no se redistribuyen aquí; verificar licencias y procedencia antes de publicarlos.

## 5. Archivos locales a sincronizar (NO confirmados en GitHub)

**Código:** `retinaface_profiling.cpp`, `retinaface_fp32.cpp`, `retinaface_no_fp16_arithmetic.cpp`, `retinaface_fp16_arithmetic_only.cpp`, `examples/CMakeLists.txt` (solo cambios relevantes; no reemplazar por una copia incompleta).

**Logs:** `retinaface_profiling_cpu.log`, `retinaface_profiling_gpu.log`, `retinaface_vulkan_layers.log`, `retinaface_fp32.log`, `fp16_round{1,2,3}.log`, `fp32_round{1,2,3}.log`, `retinaface_no_fp16_arithmetic.log`, `retinaface_fp16_arithmetic_only.log`. El benchmark de comparación de cuatro configuraciones **aún no se había ejecutado** en el punto documentado.

No subir `ncnn/build/`, binarios, librerías, archivos de terceros sin licencia revisada ni imágenes privadas.

## 6. Siguiente experimento

Repetir las cuatro variantes en tres rondas con orden alternado, guardar `fp16_comparison_{111,000,110,001}_round{1,2,3}.log`, comparar media y dispersión. Posteriormente separar `packed` y `storage`, y cuantificar explícitamente los costos de transferencias/sincronización GPU→CPU.

**Alcance:** esta bitácora registra lo comunicado en la sesión; los logs y fuentes originales locales deben sincronizarse para auditoría completa.

## 7. Validación posterior: tres rondas con orden alternado

**Ejecutado y confirmado en el Arduino**, después de redactar las secciones anteriores. Cada ejecutable se corrió tres veces con los siguientes órdenes: ronda 1 `111 → 000 → 110 → 001`; ronda 2 `001 → 110 → 000 → 111`; ronda 3 `110 → 111 → 001 → 000`. Cada ejecución produjo su propio log `~/solipsis-edge-ai/fp16_comparison_<config>_round<N>.log`.

| Configuración | Ronda 1 (ms) | Ronda 2 (ms) | Ronda 3 (ms) | Media (ms) | Rango (ms) |
|---|---:|---:|---:|---:|---:|
| 111 | 401.135 | 400.712 | 401.730 | **401.192** | 1.018 |
| 000 | 388.680 | 388.986 | 388.814 | **388.827** | 0.306 |
| 110 | 420.645 | 420.028 | 419.253 | **419.975** | 1.392 |
| 001 | 389.152 | 388.327 | 388.513 | **388.664** | 0.825 |

La diferencia de las medias entre `000` y `001` es **0.163 ms**, menor que la variación observada entre rondas. Ambas configuraciones son aproximadamente **3.1 %** más rápidas que `111`. La configuración `110` es aproximadamente **4.7 %** más lenta que `111`. Estos resultados refuerzan la asociación entre desactivar conjuntamente `packed`/`storage` y menor latencia; no separan sus efectos individuales ni demuestran una causalidad microarquitectónica.

### 7.1. Estado de los archivos locales y sincronización

La inspección local confirmó que `~/solipsis-edge-ai/ncnn/.git` tiene `origin=https://github.com/Tencent/ncnn.git`. **No hacer push a ese origin**. Los cuatro fuentes experimentales existen bajo `ncnn/examples/` y aparecen sin seguimiento (`??`); `examples/CMakeLists.txt` está modificado. También existen backups y variantes anteriores que deben conservarse o seleccionarse explícitamente, sin confundirlos con los cuatro binarios experimentales. Se verificó la existencia de los 12 logs de la validación y de logs anteriores en `~/solipsis-edge-ai/`.

**Pendiente de publicación:** copiar los archivos propios y logs seleccionados a una copia local de `ferrglez/solipsis-uno-q-edge-ai` y hacer commit/push a ese repositorio. No afirmar que las fuentes o logs están publicados antes de verificarlo. No incorporar `ncnn/build/` ni cambiar el remote del repositorio upstream.

**Nota de vigencia:** la sección 6 describía el siguiente experimento antes de ejecutarse; la sección 7 registra que ya se realizó.
