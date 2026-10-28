# Dependencias

El experimento utilizó herramientas de compilación C/C++, CMake, Git, Vulkan, OpenCV y ncnn. Las versiones y dependencias exactas instaladas deben cotejarse con el registro de terminal original.

Comandos para auditar el entorno sin modificarlo:
```bash
cmake --version
c++ --version
pkg-config --modversion opencv4
vulkaninfo --summary
```

No se afirma que este documento sea un instalador reproducible desde una imagen limpia.
