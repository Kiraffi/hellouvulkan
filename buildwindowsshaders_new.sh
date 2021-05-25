#!/bin/bash

glslc assets/shader/vulkan_new/texturedquad.vert --target-env=vulkan1.1 -o assets/shader/vulkan_new/texturedquad_vert.spv
glslc assets/shader/vulkan_new/texturedquad.frag --target-env=vulkan1.1 -o assets/shader/vulkan_new/texturedquad_frag.spv
