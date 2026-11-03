# 13 · SmolVLM2 Q8_0 y Vulkan: progreso experimental en Adreno 702

**Actualización documental:** 3 de noviembre de 2026. **Plataforma:** Arduino UNO Q, Debian 13 ARM64, Qualcomm Adreno 702, Mesa Turnip Vulkan. **Estado:** inferencia de texto con offload parcial a GPU lograda en un entorno experimental; inferencia **multimodal con imágenes totalmente acelerada en GPU, correcta y estable, sigue sin validarse**.

Esta nota continúa [el diagnóstico inicial del 28 de octubre](12-smolvlm2-vulkan-diagnostico-2026-10-28.md) y no sustituye su registro histórico. Los comandos y cifras de esta sección proceden de los resultados compartidos durante las sesiones de depuración. **No se han incorporado aún los logs completos recientes desde la placa ni se ha verificado un benchmark controlado de CPU y GPU bajo idénticas condiciones.**

## Objetivo de producto y alcance

El objetivo aplicado es un sistema local de detección facial e identificación/verificación de personas autorizadas. Los módulos son diferentes:
- **Detección facial:** RetinaFace MobileNet 0.25 con ncnn y Vulkan; implementado, incluida cámara USB y visualización MJPEG. [Documentación](11-retinaface-webcam-mjpeg.md).
- **Identificación/verificación:** requiere modelo de embeddings faciales, comparación de identidades y política de autorización; **no implementado ni validado** por RetinaFace.
- **Análisis visual con VLM:** SmolVLM2-500M Q8_0 con llama.cpp; visión CPU funcional, ejecución multimodal GPU pendiente. Un VLM no reemplaza automáticamente un modelo de reconocimiento de identidad.

## Evidencia histórica y actualización

| Caso | Resultado registrado | Interpretación |
|---|---|---|
| SmolVLM2 con imagen, CPU | Funcional; ~11.8 tokens/s de generación | Línea base multimodal CPU, según diagnóstico de octubre |
| SmolVLM2 con imagen, 99 capas GPU y proyector | `vk::Device::waitForFences: ErrorDeviceLost`, hangcheck de GPU | No constituye inferencia multimodal GPU estable |
| Texto solo, Vulkan temprano (1 capa) | Respuesta incoherente, ~0.5 tokens/s | Estado inicial, anterior a parches |
| Texto solo, offload parcial Vulkan posterior | Prueba con 10 capas GPU completó generación; alrededor de 0.4 tokens/s en ensayo de depuración | Prueba de avance funcional limitado, no benchmark final ni ejecución íntegramente GPU |
| Texto solo, CPU histórico | ~13.7 tokens/s en un ensayo previo | No comparar como medición controlada contra la fila anterior |

**Precisión del lenguaje:** `--gpu-layers 10` descarga una parte de las capas al backend Vulkan; **no implica que el VLM completo (incluido proyector visual) se ejecute en GPU**. Las respuestas a preguntas de cultura general son pruebas de generación de texto, no pruebas de visión.

## Cambios y diagnóstico de ggml-vulkan

Repo de desarrollo local (no se versiona aquí la copia de terceros): `~/solipsis-edge-ai/vlm/llama.cpp`, revisión base `e486a4b`. Se usaron compilaciones experimentales separadas y respaldos, especialmente `build-vulkan-adreno-q8-precision`.

1. Se investigaron bloqueos de GPU con muchos layers y procesamiento visual; en el kernel aparecía `hangcheck detected gpu lockup`.
2. `GGML_VK_DISABLE_FUSION=1` evitó una discrepancia severa observada en operaciones fusionadas de normalización.
3. Desactivar flash attention (`--flash-attn off`) evitó otro error numérico grande observado durante la depuración.
4. En `ggml-vulkan.cpp`, el experimento `const bool use_subgroups = false;` fuerza reducción con memoria compartida para los pipelines relevantes; mejoró las diferencias Q8_0 observadas en pruebas específicas. **Es un parche experimental, no una corrección validada upstream.**
5. Se investigaron tamaños de workgroup y especialización del shader de softmax para evitar abortos observados en ciertas configuraciones.
6. Se compararon operaciones de ggml-vulkan con resultados CPU mediante comprobaciones de precisión. El pipeline Q8_0 × F32 para matrices con pocas columnas pasa por `ggml_vk_mul_mat_vec_q_f16`, y la variante de reducción compartida remite a `mul_mat_vec_q8_0_f32_f32`, condicionado a que la entrada continúe siendo F32 en vez de Q8_1.

### Comprobación numérica pendiente

En una corrida experimental con `GGML_VK_DISABLE_FUSION=1`, `GGML_VK_DISABLE_MMVQ=1`, `--gpu-layers 10`, `--no-mmproj`, `--flash-attn off`, se registraron discrepancias de comparación entre GPU/CPU en nodos `MUL_MAT` de las capas finales, con valores de error medio normalizado aproximados:

| Nodo | Operación | `avg_err` |
|---|---|---:|
| `ffn_gate-27` | `MUL_MAT` | 0.00323449 |
| `ffn_up-27` | `MUL_MAT` | 0.0038004 |
| `ffn_out-26` | `MUL_MAT` | 0.0125203 |
| `ffn_out-27` | `MUL_MAT` | **0.0254915** |

La última comprobación provoca `GGML_ASSERT` cuando supera el umbral experimental. **Ese número no es la exactitud facial ni la tasa de error de respuestas del modelo.** La salida `exit code 134` corresponde al aborto de la comprobación, no necesariamente a `ErrorDeviceLost`.

### Experimento -O0 (no terminado)

En `vulkan-shaders-gen.cpp` se configuró temporalmente `-O0` para el shader `mul_mat_vec_q8_0_f32_f32`; las demás variantes mantienen `-O`. Se recompiló el **generador**, pero en el último paso confirmado todavía **no se había regenerado e integrado el nuevo SPIR-V en `libggml-vulkan.so` ni repetido la comparación de inferencia**.

El repositorio documenta la hipótesis y la ruta del parche, **no publica un resultado de mejora atribuido a -O0**. Los cambios locales y respaldos deben conservarse; no modificar el checkout `llama.cpp` sin revisarlos.

## Ejemplo de prueba de comparación (puede abortar intencionalmente)

```bash
cd ~/solipsis-edge-ai/vlm/llama.cpp
GGML_VK_DISABLE_FUSION=1 GGML_VK_DISABLE_MMVQ=1 \
timeout 180s ./build-vulkan-adreno-q8-precision/bin/llama-cli \
  -m ~/solipsis-edge-ai/vlm/models/smolvlm2-500m/SmolVLM2-500M-Video-Instruct-Q8_0.gguf \
  --no-mmproj --device Vulkan0 --gpu-layers 10 --flash-attn off \
  --ctx-size 512 --predict 8 --temp 0 --threads 4 \
  -p "Complete this sentence: The capital of France is" --single-turn
```

**Advertencia:** este comando utiliza una build de comprobación numérica que puede ejecutar `GGML_ASSERT` y terminar con código 134. No es un benchmark de rendimiento de producción ni una prueba multimodal.

## Limitaciones y próximos pasos

- Obtener de la placa los **logs completos** de una generación GPU correcta, la configuración efectiva, versiones, hashes y tiempos para reproducibilidad independiente.
- Verificar que el experimento `-O0` afecta al SPIR-V que realmente se selecciona y evaluar su precisión antes de tratarlo como optimización.
- Separar exactitud numérica, estabilidad de Vulkan y rendimiento; comparar CPU y GPU con modelo, prompt, número de tokens, contexto, temperatura y proyector idénticos, registrando tiempos de carga, prompt e inferencia por separado.
- Probar nuevamente imágenes con `mmproj` y escalamiento gradual de capas en GPU; evitar asumir éxito porque funcionan preguntas de texto.
- Medir CPU/GPU, transferencia y sincronización con instrumentos y proponer al proveedor un problema reproducible de mínimo tamaño.
- Evaluar una arquitectura especializada de embeddings faciales para identificación, además de RetinaFace, con consentimiento y protección de datos.

## Archivos de evidencia

**Ya disponibles en este repositorio:** benchmarks de RetinaFace, `docs/10-...`, `docs/11-...`, `docs/12-...`, y los scripts/sources versionados bajo `src/`.

**Todavía no incorporados desde el Arduino en esta actualización:** checkout/parches locales de llama.cpp, los logs `/tmp/adreno_q8_*.log`, outputs de precisión de los pasos de depuración, ni posibles resultados posteriores al paso 219. No presentar este documento como una sincronización completa de la placa.
