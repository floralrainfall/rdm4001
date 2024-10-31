#version 330 core

in vec2 f_uv;
out vec4 o_color;

uniform sampler2DMS texture0;

void main() {
  ivec2 uv = ivec2(gl_FragCoord.x, gl_FragCoord.y);
  vec4 base_color =
      (texelFetch(texture0, uv, 0) + texelFetch(texture0, uv, 1) +
       texelFetch(texture0, uv, 2) + texelFetch(texture0, uv, 3)) /
      4;
  o_color = vec4(base_color.xyz, 1.0);
}
