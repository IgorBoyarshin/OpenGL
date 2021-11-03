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


// NOTE The rendered node size is actually set with point size
static constexpr float NODE_SIZE = 0.2f;


struct World {
    bool physics_on = true;

    Vec<2> world_size;

    unsigned int nodes_size;

    // float* nodes_pos_xs_ys;
    Vec<2>* nodes_pos;
    Vec<3>* nodes_color;

    unsigned int vao_nodes[2];
    unsigned int vbo_nodes[2];
    unsigned int tfo_nodes[2];

    static constexpr unsigned int node_vertex_components = 2 + 3; // x,y + color3

    static constexpr char* const transform_variables[] = { "DataBlock.new_pos", "DataBlock.color" };
    Shader shader_node = Shader("node_sep_calc.vert", "node_point.frag", transform_variables, 2);


    World(const Vec<2>& world_size) : world_size(world_size) {
        prepare_nodes(4 * 500000);
        glPointSize(0.1f);
    }

    ~World() {
        // ::operator delete[] (nodes_pos_xs_ys, std::align_val_t(32));
        delete[] nodes_pos;
        delete[] nodes_color;
        glDeleteBuffers(1, &vbo_nodes[0]);
        glDeleteVertexArrays(1, &vao_nodes[0]);
        glDeleteBuffers(1, &vbo_nodes[1]);
        glDeleteVertexArrays(1, &vao_nodes[1]);
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
        // nodes_pos_xs_ys = new (std::align_val_t(32)) float[2 * count];
        nodes_pos = new Vec<2>[count];
        nodes_color = new Vec<3>[count];

        const auto border = 10.5f * NODE_SIZE;
        for (unsigned int i = 0; i < count; i++) {
            const auto x = getRandomUniformFloat(border, world_size[0] - border);
            const auto y = getRandomUniformFloat(border, world_size[1] - border);
            const auto color = Vec<3>{ x / world_size[0], y / world_size[1], 0.7f };
            // nodes_pos_xs_ys[i]         = pos[0];
            // nodes_pos_xs_ys[count + i] = pos[1];
            nodes_pos[i] = Vec<2>{ x, y };
            nodes_color[i] = color;
        }

        glGenVertexArrays(1, &vao_nodes[0]);
        glBindVertexArray(vao_nodes[0]);
        {
            glGenBuffers(1, &vbo_nodes[0]);
            glBindBuffer(GL_ARRAY_BUFFER, vbo_nodes[0]);
            const unsigned int size_bytes = nodes_size * node_vertex_components * sizeof(float);
            const float* nodes_data = allocate_and_init_all_nodes_data();
            // Allocate and initialize
            glBufferData(GL_ARRAY_BUFFER, size_bytes, nodes_data, GL_DYNAMIC_DRAW);
            delete[] nodes_data;

            specify_attribs_for_nodes(); // proper GL_ARRAY_BUFFER must be bound!
        }

        glGenVertexArrays(1, &vao_nodes[1]);
        glBindVertexArray(vao_nodes[1]);
        {
            glGenBuffers(1, &vbo_nodes[1]);
            glBindBuffer(GL_ARRAY_BUFFER, vbo_nodes[1]);
            const unsigned int size_bytes = nodes_size * node_vertex_components * sizeof(float);
            // Just allocate, do not initialize
            glBufferData(GL_ARRAY_BUFFER, size_bytes, nullptr, GL_DYNAMIC_COPY);

            specify_attribs_for_nodes(); // proper GL_ARRAY_BUFFER must be bound!
        }

        glBindVertexArray(0);

        glGenTransformFeedbacks(1, &tfo_nodes[0]);
        glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, tfo_nodes[0]);
        glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, vbo_nodes[0]);

        glGenTransformFeedbacks(1, &tfo_nodes[1]);
        glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, tfo_nodes[1]);
        glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, vbo_nodes[1]);

        shader_node.bind();

        const auto attractor = world_size * 0.5f;
        const auto one_over_max_attractor = 1.0f / world_size.length();
        shader_node.setUniform1f("one_over_max_attractor", one_over_max_attractor);
        shader_node.setUniform2f("attractor", attractor[0], attractor[1]);
    }

    void specify_attribs_for_nodes() const noexcept {
        // glGetAttribLocation(program, "NAME");
        {
            const auto index = 0;
            glEnableVertexAttribArray(index);
            const auto floats_per_vertex = 2; // xy
            const auto stride_bytes = 5 * sizeof(float);
            const auto offset_bytes = 0;
            glVertexAttribPointer(index, floats_per_vertex, GL_FLOAT, GL_FALSE, stride_bytes, reinterpret_cast<const void*>(offset_bytes));
        }
        {
            const auto index = 1;
            glEnableVertexAttribArray(index);
            const auto floats_per_vertex = 3; // color3
            const auto stride_bytes = 5 * sizeof(float);
            const auto offset_bytes = 2 * sizeof(float); // skip all xy
            glVertexAttribPointer(index, floats_per_vertex, GL_FLOAT, GL_FALSE, stride_bytes, reinterpret_cast<const void*>(offset_bytes));
        }
    }

    void update_and_render(float dt, const Vec<2>& cursor) noexcept {
        static auto last = get_time_micros();
        const auto p0 = get_time_micros();
        if (physics_on) do_physics(dt, cursor);
        const auto p1 = get_time_micros();
        // resubmit_nodes_vertices_pos();
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
        static unsigned int swap_index = 0;
        static bool first_render = true;

        // shader_node.bind();
        glBindVertexArray(vao_nodes[swap_index]);

        // Transform feedback goes into the other VBO
        glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, tfo_nodes[1 - swap_index]);
        glBeginTransformFeedback(GL_POINTS);

        if (first_render) {
            first_render = false;
            glDrawArrays(GL_POINTS, 0, nodes_size);
        } else {
            // This uses the size of the Transform Feedback data to specify the number of points.
            // Apart from that, the next line is equivalent to using glDrawArrays(GL_POINTS, 0, ...).
            glDrawTransformFeedback(GL_POINTS, tfo_nodes[swap_index]);
        }

        glEndTransformFeedback();

        glFlush();


        // glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, tfo_nodes[0]);
        // float feedback[10];
        // glGetBufferSubData(GL_TRANSFORM_FEEDBACK_BUFFER, 0, 10 * sizeof(float), feedback);
        // std::cout << "Feed1 = "
        //     << feedback[0] << " "
        //     << feedback[1] << " "
        //     << feedback[2] << " "
        //     << feedback[3] << " "
        //     << feedback[4] << " ;; "
        //     << feedback[5] << " "
        //     << feedback[6] << " "
        //     << feedback[7] << " "
        //     << feedback[8] << " "
        //     << feedback[9] << " "
        //     << '\n';

        // glBindVertexArray(0);
        swap_index = 1 - swap_index;
    }

    void do_physics(float dt, const Vec<2>& cursor) noexcept {
        const float speed_scaler = 5.0f * 0.0000025f;
        const auto MAX_MAGNITUDE = 2.0f;
        const auto speed_scaler_mul_max_magnitude = speed_scaler * MAX_MAGNITUDE;
        const auto speed_scaler_mul_max_magnitude_mul_dt = speed_scaler_mul_max_magnitude * dt;
        shader_node.setUniform1f("speed_scaler_mul_max_magnitude_mul_dt", speed_scaler_mul_max_magnitude_mul_dt);
    }

    float* allocate_and_init_all_nodes_data() const noexcept {
        const auto count = nodes_size * node_vertex_components;
        float* nodes_data = new float[count];
        unsigned int i = 0;

        for (unsigned int j = 0; j < nodes_size; j++, i += 5) {
            nodes_data[i + 0] = nodes_pos[j][0];
            nodes_data[i + 1] = nodes_pos[j][1];
            nodes_data[i + 2] = nodes_color[j][0];
            nodes_data[i + 3] = nodes_color[j][1];
            nodes_data[i + 4] = nodes_color[j][2];
        }

        return nodes_data;
    }

    void set_mvp(const Matrix4f& mvp) noexcept {
        shader_node.bind();
        shader_node.setUniformMat4f("u_mvp", mvp);
    }

    void set_size(const Vec<2>& size) noexcept {
        world_size = size;

        const auto attractor = world_size * 0.5f;
        const auto one_over_max_attractor = 1.0f / world_size.length();
        shader_node.setUniform1f("one_over_max_attractor", one_over_max_attractor);
        shader_node.setUniform2f("attractor", attractor[0], attractor[1]);
    }

    void flip_physics() noexcept {
        physics_on = !physics_on;
    }
};

#endif
