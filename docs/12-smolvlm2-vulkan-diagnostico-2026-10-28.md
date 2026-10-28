# 12 · SmolVLM2 multimodal en Arduino UNO Q (28-oct-2026)

## Objetivo (PENDIENTE)

**Objetivo principal: ejecutar un VLM con aceleración de la GPU Qualcomm Adreno 702 usando Vulkan.** La ejecución CPU demostrada es una línea base funcional, **no** el cierre del objetivo. Retomar la depuración GPU más adelante, sin alterar el sistema RetinaFace que ya funciona.

## Plataforma y compilación

- Arduino UNO Q; Debian 13 trixie ARM64, 4 núcleos, ~3.6 GiB RAM y ~1.8 GiB swap.
- GPU Adreno 702, Mesa Turnip Vulkan. `llama-cli --list-devices`: `Vulkan0: Turnip Adreno (TM) 702 (1833 MiB, 1833 MiB free)`. Enumerar dispositivo **no prueba** que la inferencia Vulkan funcione.
- Código `ggml-org/llama.cpp` en `~/solipsis-edge-ai/vlm/llama.cpp`, revisión `e486a4b`, compilado con Vulkan; ejecutables `build-vulkan/bin/llama-cli` y `llama-server`.
- Compilación usada:
  ```bash
  cmake -S . -B build-vulkan -DCMAKE_BUILD_TYPE=Release -DGGML_VULKAN=ON -DLLAMA_BUILD_TESTS=OFF -DLLAMA_BUILD_EXAMPLES=ON -DLLAMA_CURL=OFF
  cmake --build build-vulkan --target llama-cli llama-server -j2
  ```

## Modelo

- Repositorio: https://huggingface.co/ggml-org/SmolVLM2-500M-Video-Instruct-GGUF
- `SmolVLM2-500M-Video-Instruct-Q8_0.gguf` — 416.6 MiB.
- `mmproj-SmolVLM2-500M-Video-Instruct-Q8_0.gguf` — 103.7 MiB.
- Carpeta local: `~/solipsis-edge-ai/vlm/models/smolvlm2-500m/`.
- **No subir pesos GGUF ni archivos compilados**. Descargar desde el repositorio de origen. El `mmproj` es imprescindible para visión.
- Imagen de prueba real: `~/solipsis-edge-ai/camera-tests/retinaface_latest.jpg`, JPEG 640×480. RetinaFace no estaba ejecutándose durante estas pruebas.

## Resultados observados

| Prueba | Configuración | Resultado |
|---|---|---|
| Visión CPU | `--device none --gpu-layers 0 --no-mmproj-offload --ctx-size 2048 --threads 4` | **Funciona**; describió una persona con chaqueta azul; prompt 3.6 t/s, generación **11.8 t/s** |
| Visión Vulkan | `--device Vulkan0 --gpu-layers 99 --mmproj-offload` | **Falla**: `ErrorDeviceLost`, `mtmd_batch_encode`, abort |
| Visión híbrida | `--device Vulkan0 --gpu-layers 10 --no-mmproj-offload` | **No validada**: >5 min, salida incoherente `berg666...`; prompt 0.5 t/s, generación 0.4 t/s |
| Solo texto Vulkan | `--no-mmproj --device Vulkan0 --gpu-layers 1 --ctx-size 512` | **Incorrecto**: respuesta incoherente; prompt 0.8 t/s, generación 0.5 t/s |
| Solo texto CPU (control) | Mismo prompt y parámetros, `--no-mmproj --device none --gpu-layers 0` | **Correcto**: `The capital of France is Paris.`; prompt 20.1 t/s, generación **13.7 t/s** |

Las velocidades son observaciones de estas ejecuciones, no benchmarks generales del hardware. La respuesta visual no se contrastó manualmente con la imagen original.

### Evidencia del kernel

```text
[drm:hangcheck_handler [msm]] *ERROR* 7.0.2.0: hangcheck detected gpu lockup rb 0!
[drm:recover_worker [msm]] *ERROR* 7.0.2.0: hangcheck recover!
[drm:recover_worker [msm]] *ERROR* 7.0.2.0: offending task: llama-cli
```

`llama-cli --list-devices` volvió a enumerar la GPU tras los bloqueos, pero eso **no valida** su capacidad de ejecutar los kernels. Hipótesis pendientes: incompatibilidad entre operaciones de ggml-vulkan y Turnip/Adreno, sincronización, o problemas específicos de shaders. **No se ha identificado la causa raíz**.

## Comando reproducible: visión en CPU (funcional)

```bash
cd ~/solipsis-edge-ai/vlm/llama.cpp
./build-vulkan/bin/llama-cli \
  -m ~/solipsis-edge-ai/vlm/models/smolvlm2-500m/SmolVLM2-500M-Video-Instruct-Q8_0.gguf \
  --mmproj ~/solipsis-edge-ai/vlm/models/smolvlm2-500m/mmproj-SmolVLM2-500M-Video-Instruct-Q8_0.gguf \
  --device none --gpu-layers 0 --no-mmproj-offload \
  --ctx-size 2048 --predict 40 --temp 0 --threads 4 \
  --image ~/solipsis-edge-ai/camera-tests/retinaface_latest.jpg \
  -p "Describe this image briefly." --single-turn
```

## Trabajo pendiente — prioridad alta: GPU VLM

1. Registrar versiones exactas de kernel, Mesa/Turnip, Vulkan loader, llama.cpp y opciones de compilación.
2. Reproducir una operación de inferencia mínima con Vulkan y recopilar logs de usuario/kernel, evitando cargas prolongadas que bloqueen la GPU.
3. Contrastar resultados CPU/Vulkan con el mismo prompt, semillas, contexto y modelo; aislar si el error es específico de cuantización Q8_0, shader, operación o driver.
4. Investigar compatibilidad/bugs conocidos de ggml-vulkan con Adreno 702/Turnip; probar parches o versiones **en una compilación separada**, preservando el entorno funcional.
5. Solo cuando Vulkan produzca resultados correctos, medir rendimiento y aumentar progresivamente capas y carga visual.
6. Integrar captura Logitech C920 + RetinaFace GPU (ncnn) + SmolVLM2; por ahora usar VLM CPU como ruta estable.

**Criterio de éxito pendiente:** respuesta visual correcta, repetible y estable con trabajo del VLM realmente ejecutado en la GPU Adreno 702, sin GPU lockups.
