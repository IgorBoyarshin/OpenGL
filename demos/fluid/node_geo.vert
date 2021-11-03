#version 330 core

/* layout (location = 0) in vec2 inner_position; */
layout (location = 0) in vec2 position;
layout (location = 1) in vec3 color;

uniform mat4 u_mvp = mat4(1.0);

out VS_OUT {
    vec3 color;
} vs_out;

void main() {
    vs_out.color = color;
    gl_Position = u_mvp * vec4(position, 0.5f + 0.00000001 * gl_VertexID, 1.0);
}
