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


static constexpr float NODE_SIZE = 0.2f;


struct World {
    bool physics_on = true;

    Vec<2> world_size;

    unsigned int nodes_size;

    Vec<2>* nodes_pos;
    Vec<2>* nodes_vel;
    Vec<3>* nodes_color;

    unsigned int vao_nodes;
    unsigned int vbo_nodes;

    static constexpr unsigned int node_vertex_components = 2 + 3; // x,y + color3

    Shader shader_node{"node_point.vert", "node_point.frag"};

    float* nodes_data;


    World(const Vec<2>& world_size) : world_size(world_size) {
        glPointSize(NODE_SIZE);
        prepare_nodes(4 * 500000);
    }

    ~World() {
        delete[] nodes_data;
        delete[] nodes_pos;
        delete[] nodes_vel;
        delete[] nodes_color;
        glDeleteBuffers(1, &vbo_nodes);
        glDeleteVertexArrays(1, &vao_nodes);
    }

    float blend_unchecked(float x, float y, float t) const noexcept {
        return x * (1.0f - t) + y * t;
    }

    Vec<2> cw_dir_perp_to(const Vec<2>& dir) const noexcept {
        Vec<2> v{ dir[1], -dir[0] };
        v.normalize();
        return v;
    }

    Vec<2> speed_at(const Vec<2>& pos, const Vec<2>& attractor) const noexcept {
        const auto max_attractor = world_size;
        constexpr auto MAX_MAGNITUDE = 2.0f;
        const auto t = 1.0f - ((pos - attractor).length_sqr()) / (max_attractor.length_sqr());
        const auto magnitude = MAX_MAGNITUDE * t;
        const auto dir = pos - attractor;
        return cw_dir_perp_to(dir) * magnitude;
    }

    void prepare_nodes(unsigned int count) noexcept {
        nodes_size = count;
        nodes_pos = new Vec<2>[count];
        nodes_vel = new Vec<2>[count];
        nodes_color = new Vec<3>[count];

        const auto border = 10.5f * NODE_SIZE;
        for (unsigned int i = 0; i < count; i++) {
            const auto x = getRandomUniformFloat(border, world_size[0] - border);
            const auto y = getRandomUniformFloat(border, world_size[1] - border);
            const auto pos = Vec<2>{ x, y };
            constexpr auto MAX_VEL = 1.0f; 
            const auto vx = getRandomUniformFloat(0.0f, MAX_VEL);
            const auto vy = getRandomUniformFloat(0.0f, MAX_VEL);
            const auto vel = Vec<2>{ vx, vy };
            const auto color = Vec<3>{ x / world_size[0], y / world_size[1], 0.7f };
            nodes_pos[i] = pos;
            nodes_vel[i] = vel;
            nodes_color[i] = color;
        }

        glGenVertexArrays(1, &vao_nodes);
        glBindVertexArray(vao_nodes);
        {
            glGenBuffers(1, &vbo_nodes);
            glBindBuffer(GL_ARRAY_BUFFER, vbo_nodes);
            const unsigned int size_bytes = nodes_size * node_vertex_components * sizeof(float);
            nodes_data = allocate_and_init_all_nodes_data();
            glBufferData(GL_ARRAY_BUFFER, size_bytes, nodes_data, GL_DYNAMIC_DRAW);

            specify_attribs_for_nodes(); // proper GL_ARRAY_BUFFER must be bound!
        }

        // glBindVertexArray(0);

        shader_node.bind();
        // glBindVertexArray(vao_nodes);
    }

    void specify_attribs_for_nodes() const noexcept {
        {
            const auto index = 0;
            glEnableVertexAttribArray(index);
            const auto floats_per_vertex = 2; // x, y
            const auto stride_bytes = 0;
            const auto offset_bytes = 0;
            glVertexAttribPointer(index, floats_per_vertex, GL_FLOAT, GL_FALSE, stride_bytes, reinterpret_cast<const void*>(offset_bytes));
        }
        {
            const auto index = 1;
            glEnableVertexAttribArray(index);
            const auto floats_per_vertex = 3; // color3
            const auto stride_bytes = 0;
            const auto offset_bytes = (2 * sizeof(float)) * nodes_size; // skip x,y,z of all
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
        // static const auto bound = [this](Node& node){
        //     static constexpr auto border = 0.5f * NODE_SIZE;
        //     if ((node.pos[0] < border) || ((world_size[0] - border) < node.pos[0])) node.vel[0] = -node.vel[0];
        //     if ((node.pos[1] < border) || ((world_size[1] - border) < node.pos[1])) node.vel[1] = -node.vel[1];
        // };

        for (unsigned int i = 0; i < nodes_size; i++) {
            auto& node_pos = nodes_pos[i];
            auto& node_vel = nodes_vel[i];

            // node_vel = speed_at(node_pos, cursor);
            node_vel = speed_at(node_pos, world_size / 2.0f);
            node_pos += node_vel * (speed_scaler * dt);
        }
    }

    float* allocate_and_init_all_nodes_data() const noexcept {
        const auto count = nodes_size * node_vertex_components;
        float* nodes_data = new float[count];
        unsigned int i = 0;

        for (unsigned int j = 0; j < nodes_size; j++, i += 2) { // xyz
            nodes_data[i + 0] = nodes_pos[j][0];
            nodes_data[i + 1] = nodes_pos[j][1];
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

        const auto p1 = get_time_micros();

        for (unsigned int i = 0; i < nodes_size; i++) {
            nodes_data[i * 2 + 0] = nodes_pos[i][0];
            nodes_data[i * 2 + 1] = nodes_pos[i][1];
        }
        const auto p2 = get_time_micros();

        glBufferSubData(GL_ARRAY_BUFFER, 0, actual_size_bytes, nodes_data);
        const auto p3 = get_time_micros();

        std::cout
            << "Resubmit === "
            << "Prepare = " << (p2-p1) << "; "
            << "Upload = " << (p3-p2) << ";"
            << '\n';
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
