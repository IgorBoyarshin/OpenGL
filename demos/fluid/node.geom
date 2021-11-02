#version 330 core

layout (points) in;
layout (triangle_strip, max_vertices = 4) out;

in VS_OUT {
    vec3 color;
    float z_offset;
} gs_in[];

out vec2 v_pos;
out vec3 v_color;

void main() {
    const float off = 0.0014;
    v_color = gs_in[0].color;

    // XXX wew subtract the Z to make each subsequent oobject be behind, thus not rendered,
    // in order to achieve transparency.
    v_pos = vec2(-1.0, -1.0);
    gl_Position = gl_in[0].gl_Position + vec4(-off, -off, -gs_in[0].z_offset, 0.0);
    EmitVertex();
    v_pos = vec2(-1.0, +1.0);
    gl_Position = gl_in[0].gl_Position + vec4(-off, +off, -gs_in[0].z_offset, 0.0);
    EmitVertex();
    v_pos = vec2(+1.0, -1.0);
    gl_Position = gl_in[0].gl_Position + vec4(+off, -off, -gs_in[0].z_offset, 0.0);
    EmitVertex();
    v_pos = vec2(+1.0, +1.0);
    gl_Position = gl_in[0].gl_Position + vec4(+off, +off, -gs_in[0].z_offset, 0.0);
    EmitVertex();
    EndPrimitive();
}
