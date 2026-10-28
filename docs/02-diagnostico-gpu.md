# Diagnóstico de GPU

La consulta `vulkaninfo --summary` identificó dos dispositivos:

1. GPU0: **Turnip Adreno (TM) 702**, Mesa 25.2.6 (controlador Vulkan de hardware).
2. GPU1: **llvmpipe** (implementación por software; no equivale a aceleración física).

`clinfo` identificó Rusticl OpenCL 3.0 y dispositivo FD702 Qualcomm. La presencia de Vulkan/OpenCL no demuestra por sí sola que todas las capas de RetinaFace se ejecuten en GPU.

Comandos de diagnóstico:
```bash
vulkaninfo --summary
clinfo
```
