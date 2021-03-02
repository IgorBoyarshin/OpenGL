#version 330 core

layout (location = 0) in vec2 position;
layout (location = 1) in vec3 color;

out vec3 v_color;
out vec2 v_pos;

uniform mat4 u_MVP = mat4(1.0);

void main() {
    v_color = color;
    v_pos = position;
    gl_Position = u_MVP * vec4(position, 0.0, 1.0);
}
