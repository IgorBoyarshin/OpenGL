#ifndef WORLD_H
#define WORLD_H

#include "Shader.h"
#include "Texture.h"
#include "Vec.h"
#include "util.h"

#include <cmath>
#include <algorithm>
#include <optional>
#include <random>
#include <limits>

#include <immintrin.h>

#include <GLFW/glfw3.h>


// float, double, long double
// [low, high)
template<typename T = float>
T getRandomUniformFloat(T low, T high) {
    static std::random_device rd;
    static std::seed_seq seed{1, 2, 3, 300};
    static std::mt19937 e2(seed);
    // static std::mt19937 e2(rd());
    std::uniform_real_distribution<T> dist(low, high);

    return dist(e2);
}


// NOTE The rendered node size is actually set in geometry shader
static constexpr float NODE_SIZE = 0.2f;


struct World {
    bool physics_on = true;

    Vec<2> world_size;

    unsigned int nodes_size;

    float* nodes_pos_xs_ys;
    Vec<3>* nodes_color;

    unsigned int vao_nodes;
    unsigned int vbo_nodes;

    static constexpr unsigned int node_vertex_components = 2 + 3; // x,y + color3

    Shader shader_node{"node_geo_sep.vert", "node.geom", "node.frag"};


    World(const Vec<2>& world_size) : world_size(world_size) {
        prepare_nodes(4 * 500000);
    }

    ~World() {
        ::operator delete[] (nodes_pos_xs_ys, std::align_val_t(32));
        delete[] nodes_color;
        glDeleteBuffers(1, &vbo_nodes);
        glDeleteVertexArrays(1, &vao_nodes);
    }

    float blend_unchecked(float x, float y, float t) const noexcept {
        return x * (1.0f - t) + y * t;
    }

    // Vec<2> cw_dir_perp_to(const Vec<2>& dir) const noexcept {
    //     Vec<2> v{ dir[1], -dir[0] };
    //     v.normalize();
    //     return v;
    // }

    // Vec<2> speed_at(const Vec<2>& pos, const Vec<2>& attractor) const noexcept {
    //     const auto max_attractor = world_size;
    //     constexpr auto MAX_MAGNITUDE = 2.0f;
    //     const auto t = 1.0f - ((pos - attractor).length_sqr()) / (max_attractor.length_sqr());
    //     const auto magnitude = MAX_MAGNITUDE * t;
    //     const auto dir = pos - attractor;
    //     return cw_dir_perp_to(dir) * magnitude;
    // }

    void prepare_nodes(unsigned int count) noexcept {
        nodes_size = count;
        nodes_pos_xs_ys = new (std::align_val_t(32)) float[2 * count];
        nodes_color = new Vec<3>[count];

        const auto border = 10.5f * NODE_SIZE;
        for (unsigned int i = 0; i < count; i++) {
            const auto x = getRandomUniformFloat(border, world_size[0] - border);
            const auto y = getRandomUniformFloat(border, world_size[1] - border);
            const auto pos = Vec<2>{ x, y };
            const auto color = Vec<3>{ x / world_size[0], y / world_size[1], 0.7f };
            nodes_pos_xs_ys[i]         = pos[0];
            nodes_pos_xs_ys[count + i] = pos[1];
            nodes_color[i] = color;
        }

        glGenVertexArrays(1, &vao_nodes);
        glBindVertexArray(vao_nodes);
        {
            glGenBuffers(1, &vbo_nodes);
            glBindBuffer(GL_ARRAY_BUFFER, vbo_nodes);
            const unsigned int size_bytes = nodes_size * node_vertex_components * sizeof(float);
            const float* nodes_data = allocate_and_init_all_nodes_data();
            glBufferData(GL_ARRAY_BUFFER, size_bytes, nodes_data, GL_DYNAMIC_DRAW);
            delete[] nodes_data;

            specify_attribs_for_nodes(); // proper GL_ARRAY_BUFFER must be bound!
        }

        // glBindVertexArray(0);
        shader_node.bind();
    }

    void specify_attribs_for_nodes() const noexcept {
        {
            const auto index = 0;
            glEnableVertexAttribArray(index);
            const auto floats_per_vertex = 1; // x
            const auto stride_bytes = 0;
            const auto offset_bytes = 0;
            glVertexAttribPointer(index, floats_per_vertex, GL_FLOAT, GL_FALSE, stride_bytes, reinterpret_cast<const void*>(offset_bytes));
        }
        {
            const auto index = 1;
            glEnableVertexAttribArray(index);
            const auto floats_per_vertex = 1; // y
            const auto stride_bytes = 0;
            const auto offset_bytes = nodes_size * sizeof(float); // skip xs
            glVertexAttribPointer(index, floats_per_vertex, GL_FLOAT, GL_FALSE, stride_bytes, reinterpret_cast<const void*>(offset_bytes));
        }
        {
            const auto index = 2;
            glEnableVertexAttribArray(index);
            const auto floats_per_vertex = 3; // color3
            const auto stride_bytes = 0;
            const auto offset_bytes = 2 * nodes_size * sizeof(float); // skip xs and ys
            glVertexAttribPointer(index, floats_per_vertex, GL_FLOAT, GL_FALSE, stride_bytes, reinterpret_cast<const void*>(offset_bytes));
        }
    }

    void update_and_render(float dt, const Vec<2>& cursor) noexcept {
        static auto last = get_time_micros();
        const auto p0 = get_time_micros();
        if (physics_on) do_physics(dt, cursor);
        const auto p1 = get_time_micros();
        resubmit_nodes_vertices_pos();
        const auto p2 = get_time_micros();
        render_nodes();
        const auto p3 = get_time_micros();
        const auto frame_time = p3 - last;
        const auto fps = 1'000'000.0f / frame_time;
        std::cout
            << "Idle = " << std::setw(5) << (p0 - last) << "  "
            << "Physics = " << std::setw(5) << (p1 - p0) << "  "
            << "Resubmit = " << std::setw(5) << (p2 - p1) << "  "
            << "Render = " << std::setw(5) << (p3 - p2) << "  "
            << "Frame time = " << std::setw(5) << frame_time << "  "
            << "FPS = " << std::setw(5) << fps << '\n';
        last = p3;
    }

    void render_nodes() const noexcept {
        // shader_node.bind();
        // glBindVertexArray(vao_nodes);
        glDrawArrays(GL_POINTS, 0, nodes_size);
        // glBindVertexArray(0);
    }

    void do_physics(float dt, const Vec<2>& cursor) noexcept {
        constexpr float speed_scaler = 5.0f * 0.0000025f;

        constexpr auto MAX_MAGNITUDE = 2.0f;
        // const auto& attractor = cursor;

        // Process bulk part
        {
            const __m256 attr_x                                = _mm256_set1_ps((world_size * 0.5f)[0]);
            const __m256 attr_y                                = _mm256_set1_ps((world_size * 0.5f)[1]);
            const __m256 one_over_max_attractor                = _mm256_set1_ps(1.0f / world_size.length());
            const __m256 speed_scaler_mul_max_magnitude_mul_dt = _mm256_set1_ps(speed_scaler * MAX_MAGNITUDE * dt);
            for (unsigned int i = 0; i + 7 < nodes_size; i += 8) {
                __m256 pos_x = _mm256_load_ps(&nodes_pos_xs_ys[i]);
                const __m256 x = _mm256_sub_ps(pos_x, attr_x);
                const __m256 xx = _mm256_mul_ps(x, x);

                __m256 pos_y = _mm256_load_ps(&nodes_pos_xs_ys[nodes_size + i]);
                const __m256 y = _mm256_sub_ps(pos_y, attr_y);
                const __m256 yy = _mm256_mul_ps(y, y);

                const __m256 xx_plus_yy = _mm256_add_ps(xx, yy);
                const __m256 one_over_length = _mm256_rsqrt_ps(xx_plus_yy);

                const __m256 mul = _mm256_mul_ps(_mm256_sub_ps(one_over_length, one_over_max_attractor), speed_scaler_mul_max_magnitude_mul_dt);

                pos_x = _mm256_add_ps(pos_x, _mm256_mul_ps(y, mul));
                pos_y = _mm256_sub_ps(pos_y, _mm256_mul_ps(x, mul));

                _mm256_store_ps(&nodes_pos_xs_ys[             i], pos_x);
                _mm256_store_ps(&nodes_pos_xs_ys[nodes_size + i], pos_y);
            }
        }

        // Process remainder
        {
            const auto attractor = world_size * 0.5f;
            const auto one_over_max_attractor = 1.0f / world_size.length();
            const auto speed_scaler_mul_max_magnitude_mul_dt = speed_scaler * MAX_MAGNITUDE * dt;
            for (unsigned int i = nodes_size - (nodes_size & 0b111); i < nodes_size; i++) {
                const float x = nodes_pos_xs_ys[             i] - attractor[0];
                const float y = nodes_pos_xs_ys[nodes_size + i] - attractor[1];
                const float one_over_length = 1.0f / std::sqrt(x * x + y * y);
                const float mul = (one_over_length - one_over_max_attractor) * speed_scaler_mul_max_magnitude_mul_dt;
                nodes_pos_xs_ys[             i] += y * mul;
                nodes_pos_xs_ys[nodes_size + i] -= x * mul;
            }
        }
    }

    float* allocate_and_init_all_nodes_data() const noexcept {
        const auto count = nodes_size * node_vertex_components;
        float* nodes_data = new float[count];
        unsigned int i = 0;

        for (; i < 2 * nodes_size; i++) { // xy
            nodes_data[i] = nodes_pos_xs_ys[i];
        }

        for (unsigned int j = 0; j < nodes_size; j++, i += 3) { // color3
            nodes_data[i + 0] = nodes_color[j][0];
            nodes_data[i + 1] = nodes_color[j][1];
            nodes_data[i + 2] = nodes_color[j][2];
        }

        return nodes_data;
    }

    void resubmit_nodes_vertices_pos() const noexcept {
        glBindBuffer(GL_ARRAY_BUFFER, vbo_nodes);
        const auto count = nodes_size * 2;
        const auto actual_size_bytes = count * sizeof(float);
        glBufferSubData(GL_ARRAY_BUFFER, 0, actual_size_bytes, nodes_pos_xs_ys);
    }

    void set_mvp(const Matrix4f& mvp) noexcept {
        shader_node.bind();
        shader_node.setUniformMat4f("u_mvp", mvp);
    }

    void set_size(const Vec<2>& size) noexcept {
        world_size = size;
    }

    void flip_physics() noexcept {
        physics_on = !physics_on;
    }
};

#endif
