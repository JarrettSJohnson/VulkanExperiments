#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform UniformBufferObject
{
  mat4 projview;
  vec4 viewPos;
  vec4 lightPos;
  vec4 lightColor;
} ubo;

layout(binding = 1) uniform sampler2D texSampler;

layout(location = 0) in vec3 fragPos;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragNormal;

layout(location = 0) out vec4 outColor;

void main(){
    vec4 objectColor = texture(texSampler, fragTexCoord);

    //Ambient
    float ambientStrength = 0.9;
    vec3 ambient = ambientStrength * ubo.lightColor.xyz;
  	
  	//Diffuse
    vec3 norm = normalize(fragNormal);
    vec3 lightDir = normalize(ubo.lightPos.xyz - fragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * ubo.lightColor.xyz;
    
    //Specular
    float specularStrength = 0.2;
    vec3 viewDir = normalize(ubo.viewPos.xyz - fragPos);
    vec3 reflectDir = reflect(-lightDir, norm);  
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
    vec3 specular = specularStrength * spec * ubo.lightColor.xyz;
        
    vec3 result = (ambient + diffuse + specular) * objectColor.xyz;

    if(gl_FrontFacing){
      outColor = vec4(result, 1.0);
    } else {
      outColor = vec4(0.0, 0.0, 0.0, 1.0);
    }
}
