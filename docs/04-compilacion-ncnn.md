# Compilación de ncnn

Se compiló ncnn con soporte Vulkan y ejemplos habilitados. La compilación terminó en **813/813** pasos y generó `libncnn.a`.

Opciones de configuración relevantes:
```bash
-DNCNN_VULKAN=ON
-DNCNN_BUILD_EXAMPLES=ON
```

El código fuente y la configuración efectiva deben verificarse en el árbol de trabajo original del Arduino; este repositorio todavía no incluye el commit exacto de ncnn ni la modificación final de RetinaFace.
