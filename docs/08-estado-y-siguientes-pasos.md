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
