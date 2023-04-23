#version 430

in vec2 uv;

uniform sampler2D diffuse_texture;

out vec4 out_color;

void main()
{
   out_color = texture(diffuse_texture, uv);
}
