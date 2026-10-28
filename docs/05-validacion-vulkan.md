# Validación de Vulkan

Se probó la enumeración de GPU desde ncnn. La selección experimental para el detector fue `set_vulkan_device(0)` con `opt.use_vulkan_compute = true`.

**Precaución:** seleccionar un dispositivo Vulkan no garantiza que la ruta completa del modelo se ejecute en GPU. Se requiere instrumentación para separar cómputo, transferencias y posprocesamiento.
