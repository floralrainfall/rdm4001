#version 330 core
layout(location = 0) out vec4 f_color;
layout(location = 1) out vec4 f_bloom;

in vec4 v_fcolor;  // the input variable from the vertex shader (same name and
                   // same type)
in vec3 v_fnormal;
in vec3 v_fmpos;
in vec2 v_fuv;
in vec2 v_flm_uv;

struct light {
  vec3 ambient;
  vec3 diffuse;
  vec3 specular;
  vec3 position;
};

uniform sampler2D surface;
uniform sampler2D lightmap;
uniform samplerCube skybox;
uniform float shininess = 0.0;
uniform float gamma = 4;
uniform vec3 camera_position;

void main() {
  vec3 i = normalize(v_fmpos - camera_position);
  vec3 r = reflect(i, normalize(v_fnormal));

  vec4 samplet = texture2D(surface, vec2(v_fuv.x, -v_fuv.y));
  vec4 samplel = texture2D(lightmap, v_flm_uv) * gamma;
  vec4 samples = texture(skybox, vec3(r.x, -r.z, r.y)) * shininess;

  float intensity = dot(v_fnormal, normalize(vec3(0.5, 0.5, 0.5)));
  vec3 result = samplet.xyz * samplel.xyz + samples.xyz;
  float brightness = dot(result, vec3(0.2126, 0.7152, 0.0722));
  f_color = vec4(result, 1.0);
  if (brightness > 0.8)
    f_bloom = vec4(f_color);
  else
    f_bloom = vec4(0.0, 0.0, 0.0, 1.0);
}
