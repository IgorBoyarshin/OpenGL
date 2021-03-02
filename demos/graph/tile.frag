#version 330 core

layout (location = 0) out vec4 color;

in vec3 v_color;
in vec2 v_pos;

uniform vec2 u_Pointer = vec2(0.0, 0.0);

void main() {
    float dist = distance(u_Pointer, v_pos);
    float size = 1900.0;
    float factor = size / (dist * dist);
    color = mix(vec4(v_color, 1.0), vec4(1.0, 1.0, 1.0, 1.0), factor);
    // color = vec4(v_color, 1.0);
}
