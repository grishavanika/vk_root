set MY_glslc=C:\VulkanSDK\1.3.296.0\Bin\glslc.exe

call %MY_glslc% 09_shader_base.frag -o frag_09.spv
call %MY_glslc% 09_shader_base.vert -o vert_09.spv

call %MY_glslc% 18_shader_vertexbuffer.frag -o frag_18.spv
call %MY_glslc% 18_shader_vertexbuffer.vert -o vert_18.spv

call %MY_glslc% 22_shader_ubo.frag -o frag_22.spv
call %MY_glslc% 22_shader_ubo.vert -o vert_22.spv

call %MY_glslc% 26_shader_textures.frag -o frag_26.spv
call %MY_glslc% 26_shader_textures.vert -o vert_26.spv
