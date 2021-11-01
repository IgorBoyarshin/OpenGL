#version 330 core

layout (location = 0) in vec2 position;
layout (location = 1) in vec3 color;

uniform mat4 u_mvp = mat4(1.0);

out vec3 v_color;

void main() {
    v_color = color;
    gl_Position = u_mvp * vec4(position, 0.1f + 0.00000001 * gl_VertexID, 1.0);
}
