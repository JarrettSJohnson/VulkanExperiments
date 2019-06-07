#version 450

layout (location = 0) in vec2 position;

layout (location = 0) out vec2 outUV;

layout (binding = 0) uniform PER_VERT
{
  mat4 model;
} vtTx;

void main()
{
    outUV.x = ((position.x - -1) / (1 - -1)) * (1 - 0) + 0;
    outUV.y = ((position.y - -1) / (1 - -1)) * (1 - 0) + 0;
    gl_Position = vtTx.model * vec4(position, 0.0, 1.0);
}
