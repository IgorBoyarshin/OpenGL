#version 330 core

layout (location = 0) in vec2 inner_position;
layout (location = 1) in vec3 position;
layout (location = 2) in vec3 color;

uniform mat4 u_mvp = mat4(1.0);

out vec2 v_pos;
out vec3 v_color;

void main() {
    v_pos = inner_position;
    v_color = color;
    gl_Position = u_mvp * vec4(position, 1.0);
}
