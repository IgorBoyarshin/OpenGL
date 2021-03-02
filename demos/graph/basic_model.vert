#version 330 core

layout (location = 0) in vec3 position;
layout (location = 1) in vec4 color;

out vec4 v_color;

uniform mat4 u_MVP = mat4(1.0);
uniform mat4 u_Model = mat4(1.0);

void main() {
    v_color = color;
    gl_Position = u_MVP * u_Model * vec4(position, 1.0);
}
