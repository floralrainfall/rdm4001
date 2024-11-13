#version 330 core

in vec2 f_uv;
out vec4 o_color;

uniform sampler2DMS texture0;
uniform sampler2DMS texture1;

uniform float exposure = 1.0;
uniform float time;

void main() {
  const float gamma = 1.0;
  ivec2 uv = ivec2(gl_FragCoord.x, gl_FragCoord.y);
  /*vec4 base_color =
      (texelFetch(texture0, uv, 0) + texelFetch(texture0, uv, 1) +
       texelFetch(texture0, uv, 2) + texelFetch(texture0, uv, 3)) /
      4;*/
  vec3 base_color = texelFetch(texture0, uv, 0).rgb;
  vec3 bloom_color = texelFetch(texture1, uv, 0).rgb;
  base_color += bloom_color;
  vec3 result = vec3(1.0) - exp(-base_color * exposure);
  result = pow(result, vec3(1.0 / gamma));
  o_color = vec4(result, 1.0);
}
