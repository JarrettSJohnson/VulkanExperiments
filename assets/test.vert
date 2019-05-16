#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texCoord;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;

void main() {
	//gl_Position = vec4(position, 0.0, 1.0);
	gl_Position = ubo.proj * ubo.view * ubo.model * vec4(position, 1.0);
    //fragColor = normal * 0.5 + 0.5;
    fragColor = fragColor;
    fragTexCoord = texCoord;
}
