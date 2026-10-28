#!/usr/bin/env bash
set -euo pipefail
mkdir -p "$HOME/solipsis-edge-ai/models/retinaface"
cd "$HOME/solipsis-edge-ai/models/retinaface"
curl -fL --retry 3 -o mnet.25-opt.param https://raw.githubusercontent.com/nihui/ncnn-assets/master/models/mnet.25-opt.param
curl -fL --retry 3 -o mnet.25-opt.bin https://raw.githubusercontent.com/nihui/ncnn-assets/master/models/mnet.25-opt.bin
curl -fL --retry 3 -o test.jpg https://raw.githubusercontent.com/opencv/opencv/master/samples/data/lena.jpg
file mnet.25-opt.* test.jpg
