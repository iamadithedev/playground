#version 430

layout (location = 0) in vec3 in_position;
layout (location = 1) in vec3 in_normal;

layout (binding = 0, std140) uniform u_matrices
{
   mat4 model;
   mat4 view;
   mat4 proj;
};

out vec3 position;
out vec3 normal;

void main()
{
    vec4 pos    = model * vec4(in_position, 1.0);
    gl_Position = proj  * view * pos;

    //normal = mat3(transpose(inverse(model))) * in_normal;
    normal   = mat3(model) * in_normal;
    position = pos.xyz;
}