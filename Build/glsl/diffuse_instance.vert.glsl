#version 460

layout (location = 0) in vec3 in_position;
layout (location = 1) in vec3 in_normal;

layout (location = 0) out vec3 out_position;
layout (location = 1) out vec3 out_normal;

layout (binding = 0, std140) uniform u_matrices
{
   mat4 model;
   mat4 view;
   mat4 proj;
};

layout (binding = 3, std140) uniform u_matrices_instance
{
    mat4 models[9];
};

void main()
{
    vec4 position = models[gl_InstanceID] * vec4(in_position, 1.0);
    gl_Position   = proj  * view * position;

    //out_normal = mat3(transpose(inverse(model))) * in_normal;
    out_normal   = mat3(model) * in_normal;
    out_position = position.xyz;
}