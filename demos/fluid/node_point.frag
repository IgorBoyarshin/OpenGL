#version 330 core

layout (location = 0) out vec4 color;

in vec3 v_color;

void main(void) {
    color = vec4(v_color, 1.0);
}
