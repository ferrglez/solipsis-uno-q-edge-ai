# Technical brief for Qualcomm — Adreno 702 Edge AI

**Status:** draft evidence summary, updated 3 November 2026. **Purpose:** request reproducible optimization guidance; do not claim definitive CPU/GPU comparisons or resolved GPU VLM.

## Project

- Hardware: Arduino UNO Q, Qualcomm Adreno 702 GPU.
- Linux: Debian 13 ARM64; Mesa Turnip Vulkan. Verify full software/kernel versions on device before sending.
- Use case: local facial detection, future face embedding identification/authorization, optional VLM scene interpretation.
- Face detection: RetinaFace MobileNet 0.25, ncnn + Vulkan, including webcam/MJPEG prototype.
- VLM: SmolVLM2-500M-Video-Instruct Q8_0, llama.cpp (baseline checkout `e486a4b`) and GGUF visual projector.

## Observed results (distinct workloads, not interchangeable)

| Workload | CPU | GPU | Evidence and caveat |
|---|---:|---:|---|
| RetinaFace still image, 10 iterations, discard first | **68.868 ms** | **395.478 ms** | Repo benchmark `docs/07-benchmarks.md`; detector timing, GPU slower by ~5.74× in this experiment |
| RetinaFace continuous webcam | Not measured as paired control | **~475–513 ms/frame**, ~2 FPS | `docs/11-retinaface-webcam-mjpeg.md`; GPU inference estimate, not formal end-to-end video FPS |
| SmolVLM2 multimodal image inference | **~11.8 generation tokens/s** | **Not yet validated** | `docs/12-smolvlm2-vulkan-diagnostico-2026-10-28.md`; GPU/mmproj previously `ErrorDeviceLost` |
| SmolVLM2 text only, historical comparison | **~13.7 generation tokens/s** | **~0.4 tokens/s**, partial GPU offload | Development observations; not paired controlled benchmark and not a full GPU VLM result |

**Correction to earlier draft language:** The original repository benchmark **does not support** the numbers 81–86 ms CPU versus 45–50 ms GPU for RetinaFace. Do not include those as verified end-to-end latencies. The source-controlled measurements report the GPU slower.

## Issues to reproduce or investigate

1. `ErrorDeviceLost` and Adreno GPU hangcheck during heavy Vulkan VLM/visual projector workload.
2. Numerically incorrect Vulkan output in quantized `MUL_MAT` and some fused operations in earlier configurations.
3. Experimental mitigation with disabled Vulkan fusion, disabled flash attention, and shared-memory reductions; not independently upstream-validated.
4. Remaining Q8_0 × F32 discrepancy: `ffn_out-27 MUL_MAT avg_err=0.0254915` in a numerical debug build, with assertion; metric is *not* facial recognition error.
5. Partial Vulkan offload can generate text but has much worse throughput than CPU in exploratory runs.
6. RetinaFace Vulkan workload is slower than CPU in the source-controlled benchmark; GPU transfers, synchronization, and layer-level profiling remain relevant.

## Questions for Qualcomm

- Which supported driver/runtime and precision/quantization configurations are recommended for **Adreno 702** on this Linux platform?
- Is Mesa Turnip Vulkan supported or recommended for production inference in this configuration, and are there known limitations around subgroup reductions, integer dot product, shader optimization, or resource management?
- What is the recommended acceleration stack for compact VLMs (llama.cpp Vulkan, ONNX Runtime/QNN, Qualcomm AI Engine Direct or another supported path) **on this exact board / OS**? Please specify version and feasibility rather than assuming SDK support.
- Which profiling tools and metrics should we use to isolate CPU↔GPU transfers, synchronization, memory bandwidth and shader execution for ncnn and ggml?
- What are the recommended resources for numerical comparison of Q8_0 kernels, Vulkan device-loss analysis, and minimal reproducible examples?
- Which on-device face embedding / verification approach is supported or optimized for the target platform?

## Evidence to attach before sending

- Firmware/board identifiers, kernel, Mesa, Vulkan ICD, runtime and ncnn/llama.cpp git revisions.
- Minimal model/prompt and exact reproduction commands, traces of `ErrorDeviceLost`, dmesg GPU hangcheck, ggml numerical check logs.
- Identical-input, paired CPU/GPU latency and generation runs including warmup and variability.
- A code diff/patch series of local Vulkan backend changes; experimental modifications should not be described as fixes.
- Security/privacy review before sharing camera frames, user data or private logs.

See [ongoing VLM investigation](13-vlm-vulkan-progreso-2026-11-03.md), [original VLM diagnosis](12-smolvlm2-vulkan-diagnostico-2026-10-28.md), [RetinaFace benchmarks](07-benchmarks.md) and [Vulkan profiling](10-profiling-vulkan-fp16-2026-10-28.md).
