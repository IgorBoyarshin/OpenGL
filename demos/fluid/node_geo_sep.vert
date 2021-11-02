#version 330 core

layout (location = 0) in float position_x;
layout (location = 1) in float position_y;
layout (location = 2) in vec3 color;

uniform mat4 u_mvp = mat4(1.0);

out VS_OUT {
    vec3 color;
    float z_offset;
} vs_out;

void main() {
    vs_out.color = color;
    vs_out.z_offset = 0.00000001 * gl_VertexID;
    gl_Position = u_mvp * vec4(position_x, position_y, 0.1f, 1.0);
}
