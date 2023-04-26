#version 430

layout (location = 0) in vec2 in_uv;

uniform sampler2D diffuse_texture;

layout (location = 0) out vec4 out_color;

void main()
{
   out_color = texture(diffuse_texture, in_uv);
}
