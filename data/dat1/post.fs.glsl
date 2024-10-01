#version 330 core 

in vec2 f_uv;
out vec4 o_color;

uniform sampler2D texture0;

void main() {
  vec4 base_color = texture(texture0, f_uv);
  o_color = vec4(base_color.xyz, 1.0);
}