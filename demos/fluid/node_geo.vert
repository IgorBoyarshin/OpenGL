#version 330 core

/* layout (location = 0) in vec2 inner_position; */
layout (location = 0) in vec2 position;
layout (location = 1) in vec3 color;

uniform mat4 u_mvp = mat4(1.0);

/* out vec2 v_pos; */
/* out vec3 v_color; */
out VS_OUT {
    vec3 color;
    float z_offset;
} vs_out;

void main() {
    /* v_pos = inner_position; */
    /* v_color = color; */
    vs_out.color = color;
    vs_out.z_offset = 0.00000001 * gl_VertexID;
    gl_Position = u_mvp * vec4(position, 0.1f, 1.0);
}
