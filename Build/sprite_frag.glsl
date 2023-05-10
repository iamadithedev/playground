#version 430

layout (location = 0) in vec2 in_uv;
layout (location = 0) out vec4 out_color;

uniform sampler2D diffuse_texture;

void main()
{
   out_color = texture(diffuse_texture, in_uv);
}
