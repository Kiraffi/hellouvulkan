rem ctrl + alt + n for coderunner

rem glslangValidator huom -V --target-env valiluonnilla
rem ~/vulkansdk/1.1.126.0/x86_64/bin/glslangValidator assets/shader/shader_ubo.vert -V --target-env vulkan1.1 -o assets/shader/vert_2.spv

rem glslc huom ei V ja --target-env=
rem ~/vulkansdk/1.1.126.0/x86_64/bin/glslc assets/shader/shader_ubo.vert --target-env=vulkan1.1 -o assets/shader/vert_2.spv

glslc assets/shader/vulkan/shader_ubo.vert --target-env=vulkan1.1 -o assets/shader/vulkan/vert_2.spv
glslc assets/shader/vulkan/shader_ubo.frag --target-env=vulkan1.1 -o assets/shader/vulkan/frag_2.spv

glslc assets/shader/vulkan/shader_static.vert --target-env=vulkan1.1 -o assets/shader/vulkan/vert_1.spv
glslc assets/shader/vulkan/shader_static.frag --target-env=vulkan1.1 -o assets/shader/vulkan/frag_1.spv

glslc assets/shader/vulkan/triangle.vert --target-env=vulkan1.1 -o assets/shader/vulkan/triangle_vert.spv
glslc assets/shader/vulkan/triangle.frag --target-env=vulkan1.1 -o assets/shader/vulkan/triangle_frag.spv

glslc assets/shader/vulkan/compute.comp --target-env=vulkan1.1 -o assets/shader/vulkan/compute_comp.spv
glslc assets/shader/vulkan/computereset.comp --target-env=vulkan1.1 -o assets/shader/vulkan/computereset_comp.spv
glslc assets/shader/vulkan/computecarpwrite.comp --target-env=vulkan1.1 -o assets/shader/vulkan/computecarpwrite_comp.spv
glslc assets/shader/vulkan/compute_create_instances.comp --target-env=vulkan1.1 -o assets/shader/vulkan/compute_create_instances_comp.spv
pause