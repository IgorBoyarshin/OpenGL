#version 330 core

layout (points) in;
layout (triangle_strip, max_vertices = 4) out;

in VS_OUT {
    vec3 color;
} gs_in[];

out vec2 v_pos;
out vec3 v_color;

void main() {
    const float off = 0.0014;
    v_color = gs_in[0].color;

    v_pos = vec2(-1.0, -1.0);
    gl_Position = gl_in[0].gl_Position + vec4(-off, -off, 0.0, 0.0);
    EmitVertex();
    v_pos = vec2(-1.0, +1.0);
    gl_Position = gl_in[0].gl_Position + vec4(-off, +off, 0.0, 0.0);
    EmitVertex();
    v_pos = vec2(+1.0, -1.0);
    gl_Position = gl_in[0].gl_Position + vec4(+off, -off, 0.0, 0.0);
    EmitVertex();
    v_pos = vec2(+1.0, +1.0);
    gl_Position = gl_in[0].gl_Position + vec4(+off, +off, 0.0, 0.0);
    EmitVertex();
    EndPrimitive();
}
