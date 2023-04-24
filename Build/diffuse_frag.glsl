#version 430

layout (location = 0) in vec3 in_position;
layout (location = 1) in vec3 in_normal;

layout (binding = 1, std140) uniform u_material
{
   vec3 diffuse;
} material;

layout (binding = 2, std140) uniform u_light
{
    vec3  position;
    float temp;
    vec3  color;
} light;

layout (location = 0) out vec4 out_color;

void main()
{
    float ambient       = 0.3f;
    vec3  ambient_color = ambient * light.color;

    vec3 normal    = normalize(in_normal);
    vec3 direction = normalize(light.position - in_position);

    float diffuse       = max(dot(normal, direction), 0.0);
    vec3  diffuse_color = diffuse * light.color;

    vec3 color = (ambient_color + diffuse_color) * material.diffuse;
    out_color  = vec4(color, 1.0);
}
