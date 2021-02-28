#version 330 core

layout (location = 0) in vec3 position;
layout (location = 1) in vec2 texCoord;

out vec2 v_TexCoord;

uniform mat4 u_MVP = mat4(1.0);
uniform mat4 u_Model = mat4(1.0);

void main() {
    v_TexCoord = texCoord;
    gl_Position = u_MVP * u_Model * vec4(position, 1.0);
}
