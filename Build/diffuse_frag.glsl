#version 430

in vec3 position;
in vec3 normal;

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

out vec4 out_color;

void main()
{
    // ==================================================================================

    float ambient_strength = 0.3f;
    vec3  ambient_color    = ambient_strength * light.color;

    // ==================================================================================

    vec3 normal    = normalize(normal);
    vec3 direction = normalize(light.position - position);

    float diffuse_strength = max(dot(normal, direction), 0.0);
    vec3  diffuse_color    = diffuse_strength * light.color;

    // ==================================================================================

    vec3 result = (ambient_color + diffuse_color) * material.diffuse;
    out_color   = vec4(result, 1.0);
}
