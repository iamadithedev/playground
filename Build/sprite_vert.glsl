#version 430

layout (location = 0) in vec2 in_position;
layout (location = 1) in vec2 in_uv;

layout (location = 0) out vec2 out_uv;

layout (binding = 0, std140) uniform u_matrices
{
   mat4 model;
   mat4 view;
   mat4 proj;
};

void main()
{
   gl_Position = proj * view * model * vec4(in_position, 0.0, 1.0);

   out_uv = in_uv;
}