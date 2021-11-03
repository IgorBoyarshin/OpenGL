#version 330 core

layout (location = 0) in vec2 position;
layout (location = 1) in vec3 color;

uniform mat4 u_mvp = mat4(1.0);

uniform float one_over_max_attractor = 1.0f;
uniform float speed_scaler_mul_max_magnitude_mul_dt = 1.0f;
uniform vec2 attractor = vec2(1.0f, 1.0f);

out vec3 v_color;

out DataBlock {
    vec2 new_pos;
    vec3 color;
} data_block;

void main() {
    v_color = color;

    float x = position.x - attractor.x;
    float y = position.y - attractor.y;
    float one_over_length = inversesqrt(x * x + y * y);
    float mul = (one_over_length - one_over_max_attractor) * speed_scaler_mul_max_magnitude_mul_dt;
    vec2 new_pos = vec2(position.x + y * mul, position.y - x * mul);
    data_block.new_pos = new_pos;
    data_block.color = color;

    gl_Position = u_mvp * vec4(new_pos, 0.5f - 0.00000001 * gl_VertexID, 1.0);
}
