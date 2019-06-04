#version 450
#extension GL_ARB_separate_shader_objects : enable

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
  int charIdx;
} pc;

layout(binding = 1) uniform sampler2D texSampler[16];

layout(location = 0) in vec3 fragPos;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragNormal;

layout(location = 0) out vec4 outColor;

void main(){
    vec4 objectColor = texture(texSampler[pc.charIdx], fragTexCoord);

    //if(!gl_FrontFacing){
   //   outColor = vec4(0.0, 0.0, 0.0, 1.0);
   //} else {
      outColor = objectColor;
   // }
}
