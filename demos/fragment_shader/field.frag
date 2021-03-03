#version 330 core

layout (location = 0) out vec4 color;

uniform vec2  u_resolution;
uniform float u_time;


#define beacons_count 3
vec2 position_of_beacon(int n, float time) {
    switch (n) {
        /* case 0: return vec2(-0.5, -0.5); */
        /* case 1: return vec2(-0.5,  0.5); */
        case 0: {
            const float r = 0.5;
            const float speed = 1.3;
            float x = cos(speed * time) * r;
            float y = sin(speed * (time + 2.1)) * r;
            return vec2(x - 0.5, y - 0.5);
        }
        case 1: {
            const float r = 0.5;
            const float speed = 1.9;
            float x = cos(speed * (time + 1.5)) * r;
            float y = sin(speed * time) * r;
            return vec2(x - 0.5, y + 0.5);
        }
        case 2: {
            const float r = 0.5;
            const float speed = 1.0;
            float x = cos(speed * time) * r;
            float y = sin(speed * time) * r;
            return vec2(x + 0.5, y);
        }
        default: return vec2(0.0, 0.0);
    }
}

float func(float x) {
    const float p = 1.9;
    return 1.0 / pow(x, p);
}


#define PI 3.14159265358979323846

void main(void){
    vec2 st = gl_FragCoord.xy / u_resolution.xy;
    st = st * vec2(2.0, 2.0) - vec2(1.0, 1.0);

    /* vec3 d; */
    /* float dist; */
    /* dist = distance(st, position_of_beacon(0, u_time)); */
    /* d.x = 1.0 / pow(dist, p); */
    /* dist = distance(st, position_of_beacon(1, u_time)); */
    /* d.y = 1.0 / pow(dist, p); */
    /* dist = distance(st, position_of_beacon(2, u_time)); */
    /* d.z = 1.0 / pow(dist, p); */

    const float factor = 0.0;
    vec3 d = vec3(0.0);
    d += vec3(func(distance(st, position_of_beacon(0, u_time))), factor, factor);
    d += vec3(factor, func(distance(st, position_of_beacon(1, u_time))), factor);
    float a = func(distance(st, position_of_beacon(2, u_time)));
    d += vec3(a, a, 0.0);
    /* float s = smoothstep(5.0, 10.0, a) * a; */
    /* float s2 = a - smoothstep(5.0, 10.0, a) * a; */
    /* if (a > 10.0) { */
    /*     d += vec3(0.0, 0.0, s); */
    /* } else { */
    /*     d += vec3(s2, s2, 0.0); */
    /* } */


    const float sensitivity = 14.0;
    d /= sensitivity;
    color = vec4(d, 1.0);

    /* float d = 0.0; */
    /* for (int i = 0; i < beacons_count; i++) { */
    /*     float dist = distance(st, position_of_beacon(i, u_time)); */
    /*     d += 1.0 / (dist * dist); */
    /* } */
    /* d /= 20.0; */
    /* color = vec4(d, d, d, 1.0); */
}
