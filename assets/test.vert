#version 450
#extension GL_ARB_separate_shader_objects : enable

//layout(binding = 0) uniform UniformBufferObject {
//    mat4 mvp;
//} ubo;

layout(binding = 0) uniform UniformBufferObject
{
  mat4 projview;
  vec4 viewPos;
  vec4 lightPos;
  vec4 lightColor;
} ubo;

layout(push_constant) uniform PER_OBJECT 
{ 
  mat4 model;
} pc;

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texCoord;

layout(location = 0) out vec3 fragPos;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec3 fragNormal;

void main() {
	fragPos = vec3(pc.model * vec4(position, 1.0));
	fragTexCoord = texCoord;
	fragNormal = mat3(transpose(inverse(pc.model))) * normal;

	gl_Position = ubo.projview * pc.model * vec4(position, 1.0);
}
