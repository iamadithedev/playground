#version 430

layout (location = 0) in vec3 in_position;
layout (location = 1) in vec2 in_uv;

layout (binding = 0, std140) uniform u_matrices
{
   mat4 model;
   mat4 view;
   mat4 proj;
};

out vec2 uv;

void main()
{
   gl_Position = proj * view * model * vec4(in_position, 1.0);

   uv = in_uv;
}