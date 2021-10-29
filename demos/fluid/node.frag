#version 330 core

layout (location = 0) out vec4 color;

in vec2 v_pos;
in vec3 v_color;

void main(void) {
    const float outter = 1.0;
    const float inner = 0.6;
    float d = distance(vec2(0.0), v_pos);
    color = vec4(v_color, (1.0 - smoothstep(inner, outter, d)));

    /* color = vec4(v_color, 1.0) * (1.0 - smoothstep(inner, outter, d)); */
    /* if (color.w < 0.14) discard; */
}
