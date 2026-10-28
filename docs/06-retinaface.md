# Detección facial con RetinaFace

Modelo utilizado: RetinaFace MobileNet 0.25 (`mnet.25-opt.param` y su binario). Imagen de prueba: Lena, 512 × 512.

Se adaptó `examples/retinaface.cpp` para:
- Seleccionar GPU0 con `retinaface.set_vulkan_device(0)` y `retinaface.opt.use_vulkan_compute = true`.
- Permitir elección `cpu` o `gpu` desde línea de comandos.
- Guardar `retinaface_result.jpg` con `cv::imwrite` en entorno SSH sin GUI.
- Medir diez iteraciones con calentamiento.

Se detectó un rostro con confianza aproximada **0.98340** (GPU) y **0.98351** (CPU).

**Pendiente:** incorporar el archivo `retinaface.cpp` modificado desde el Arduino. Esta nota describe la prueba pero no sustituye al código fuente definitivo.
