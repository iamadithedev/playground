#version 430

layout (binding = 1, std140) uniform u_material
{
   vec3 diffuse;
};

out vec4 out_color;

void main()
{
   out_color = vec4(diffuse, 1.0);
}
