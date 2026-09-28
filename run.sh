#!/bin/bash

export GALLIUM_DRIVER=d3d12 MESA_D3D12_DEFAULT_ADAPTER_NAME=NVIDIA MESA_GL_VERSION_OVERRIDE=4.6 MESA_GLSL_VERSION_OVERRIDE=460

make
cmake .

# Use prime-run (NVIDIA offload) if it's installed, otherwise run directly
if command -v prime-run > /dev/null; then
    prime-run ./raytracer
else
    ./raytracer
fi