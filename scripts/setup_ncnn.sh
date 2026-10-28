#!/usr/bin/env bash
set -euo pipefail
# Referencia reproducible; revisar antes de ejecutar. Instala paquetes del sistema.
sudo apt update
sudo apt install -y build-essential cmake ninja-build pkg-config libvulkan-dev glslang-tools libopencv-dev
mkdir -p "$HOME/solipsis-edge-ai"
cd "$HOME/solipsis-edge-ai"
if [ ! -d ncnn/.git ]; then git clone --recursive https://github.com/Tencent/ncnn.git; fi
cd ncnn
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DNCNN_VULKAN=ON -DNCNN_BUILD_EXAMPLES=ON -DNCNN_BUILD_TOOLS=OFF -DNCNN_BUILD_TESTS=OFF
cmake --build build --parallel 2
cmake --install build --prefix "$HOME/solipsis-edge-ai/ncnn-install"
