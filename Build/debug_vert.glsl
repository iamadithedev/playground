#version 430

layout (location = 0) in vec3 in_position;
layout (location = 1) in vec3 in_color;

layout (binding = 0, std140) uniform u_matrices
{
   mat4 model;
   mat4 view;
   mat4 proj;
};

layout (location = 0) out vec3 out_color;

void main()
{
    vec4 position = model * vec4(in_position, 1.0);
    gl_Position   = proj  * view * position;

    out_color = in_color;
}