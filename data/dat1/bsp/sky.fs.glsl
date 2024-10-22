#version 330 core
out vec4 f_color;
  
in vec4 v_fcolor; // the input variable from the vertex shader (same name and same type)  
in vec3 v_fnormal;
in vec3 v_fmpos;
in vec2 v_fuv;
in vec2 v_flm_uv;

uniform vec3 camera_position;
uniform float time;

float mod289(float x){return x - floor(x * (1.0 / 289.0)) * 289.0;}
vec4 mod289(vec4 x){return x - floor(x * (1.0 / 289.0)) * 289.0;}
vec4 perm(vec4 x){return mod289(((x * 34.0) + 1.0) * x);}

float noise(vec3 p){
    vec3 a = floor(p);
    vec3 d = p - a;
    d = d * d * (3.0 - 2.0 * d);

    vec4 b = a.xxyy + vec4(0.0, 1.0, 0.0, 1.0);
    vec4 k1 = perm(b.xyxy);
    vec4 k2 = perm(k1.xyxy + b.zzww);

    vec4 c = k2 + a.zzzz;
    vec4 k3 = perm(c);
    vec4 k4 = perm(c + 1.0);

    vec4 o1 = fract(k3 * (1.0 / 41.0));
    vec4 o2 = fract(k4 * (1.0 / 41.0));

    vec4 o3 = o2 * d.z + o1 * (1.0 - d.z);
    vec2 o4 = o3.yw * d.x + o3.xz * (1.0 - d.x);

    return o4.y * d.y + o4.x * (1.0 - d.y);
}

float getCloud(in vec3 ro, in vec3 rd) {
     float updot = dot(rd, vec3(0.0, 1.0, 0.0));
     float timed = 4;
     vec3 cloudPos = (ro / 100) + vec3(rd.x + (time / timed), rd.y + (time / timed), rd.z + (time / timed));
     float cloudNoise = noise(cloudPos * 2) * max(0.0, updot);
     float cloudNoise2 = noise(cloudPos * 4 + vec3(time,0,0)) * max(0.0, updot - 0.7);
     return cloudNoise + cloudNoise2;
}

uniform mat4 viewMatrix;

void main()
{
    vec3 p = normalize(v_fmpos);
    vec3 i = reflect(p, normalize(v_fnormal));
    // vec3 x = inverse(mat3(viewMatrix)) * p;
    vec3 d = normalize(camera_position + i);
    float updot = dot(i, vec3(0.0, 0.0, 1.0));
    vec3 result = mix(vec3(0.77,0.79,1.0),vec3(0.16,0.20,0.96),updot);
    float cloudNoise = getCloud(camera_position, i);
    result = mix(result,vec3(1,1,1),max(0.0,cloudNoise));
    float sundot = dot(i, normalize(vec3(0.5,0.5,0.5)));
    if(sundot > 0.9995)
        result = vec3(1.0, 1.0, 1.0);

    f_color = vec4(result, 1.0);
} 
