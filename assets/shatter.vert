#version 450

layout(location = 0) in vec2 position;

layout (location = 0) out vec2 outUV;

layout(push_constant) uniform PER_OBJECT 
{ 
  mat4 transform;
} pc;

void main()
{
    outUV = position.xy; //FIX THIS [-1, 1] to [0, 1]
	gl_Position = pc.transform * vec4(position, 0.0, 1.0);
}
