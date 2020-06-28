#!/bin/sh

glslc --target-env=vulkan1.2 -O shader.vert -o vert.spv
glslc --target-env=vulkan1.2 -O shader.frag -o frag.spv

# Or, if using glslangValidator
# glslangValidator --target-env vulkan1.2 shader.vert -o vert.spv
# glslangValidator --target-env vulkan1.2 shader.frag -o frag.spv
