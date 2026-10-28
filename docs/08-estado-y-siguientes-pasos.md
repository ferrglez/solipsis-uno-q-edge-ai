# Estado y siguientes pasos

## Confirmado
- Arduino UNO Q: Debian 13 ARM64, GPU Turnip Adreno 702 identificada con Vulkan.
- ncnn compilado con soporte Vulkan.
- RetinaFace MobileNet 0.25 ejecutado en CPU y GPU sobre imagen de prueba.
- Un rostro detectado con confianza ≈0.9834.
- Benchmark: CPU 68.868 ms y GPU 395.478 ms de media, descartando primera iteración.

## Pendiente
1. Recuperar el archivo final `examples/retinaface.cpp` directamente del Arduino.
2. Medir por separado `create_extractor`, `ex.input`, `ex.extract`, generación de propuestas y NMS.
3. Confirmar qué operaciones corren realmente en GPU y los costos de transferencia.
4. Medir rendimiento con cámara, energía, temperatura y carga sostenida.
5. Revisar la transcripción original antes de incorporarla al repositorio.

## Próximo comando previsto (no ejecutado)
```bash
grep -nE 'create_extractor|ex.input|ex.extract|generate_proposals|nms_sorted_bboxes|ITERATION|AVERAGE' ~/solipsis-edge-ai/ncnn/examples/retinaface.cpp
```

## Actualización 28-oct-2026

El plan anterior de instrumentar `ex.extract` y el posprocesamiento ya se ejecutó; también se habilitó `NCNN_BENCHMARK` para medir 90 capas Vulkan y se compararon cuatro combinaciones de opciones FP16. Consultar [bitácora de profiling y FP16](10-profiling-vulkan-fp16-2026-10-28.md) para procedimientos, cifras y limitaciones. **Pendiente prioritario:** sincronizar desde el Arduino los cuatro fuentes experimentales, cambios de CMake y logs; después repetir cuatro configuraciones en órdenes alternados. El listado anterior se conserva como estado histórico, no como tareas aún pendientes.
