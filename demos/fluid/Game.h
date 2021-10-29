#ifndef GAME_H
#define GAME_H

#include "Shader.h"
#include "Texture.h"
#include "Vec.h"

#include <cmath>
#include <algorithm>
#include <optional>
#include <random>
#include <limits>

#include <GLFW/glfw3.h>


// [signed, unsigned]: short, int, long, long long
// [low, high]
template<typename T = unsigned int>
T getRandomUniformInt(T low, T high) {
    static std::random_device rd;
    static std::seed_seq seed{1, 2, 3, 300};
    static std::mt19937 e2(seed);
    // static std::mt19937 e2(rd());
    std::uniform_int_distribution<T> dist(low, high);

    return dist(e2);
}

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


Vector3f random_color() noexcept {
    const float r = getRandomUniformFloat(0.1f, 0.7f);
    const float g = getRandomUniformFloat(0.1f, 0.7f);
    const float b = getRandomUniformFloat(0.1f, 0.7f);
    return Vector3f{ r, g, b };
}

unsigned int random_id() noexcept {
    static unsigned int next = 4;
    return next++;
    // return getRandomUniformInt(static_cast<unsigned int>(0), ID_MAX);
}


static constexpr unsigned int ID_MAX = 0xFFFFFFF;


static constexpr float NODE_SIZE = 1.0f;
struct Node {
    Vec<2> pos;
    Vec<2> vel;
};


struct World {
    Vec<2> world_size;

    std::vector<Node> nodes;

    unsigned int vao_nodes;
    unsigned int vbo_nodes;
    unsigned int ibo_nodes;

    static constexpr unsigned int node_vertex_components = 2 + 3 + 3; // inner_xy + x,y,z + color3
    static constexpr unsigned int indices_per_node = 6;
    static constexpr unsigned int vertices_per_node = 4; // regular square that turns into circle thanks to shader

    Shader shader_node{"node.vert", "node.frag"};
    static constexpr float z_nodes = 0.3f;
    static constexpr float z_delta = 0.00001f;


    World(const Vec<2>& world_size) : world_size(world_size) {
        prepare_nodes(50000);
    }

    ~World() {
        glDeleteBuffers(1, &ibo_nodes);
        glDeleteBuffers(1, &vbo_nodes);
        glDeleteVertexArrays(1, &vao_nodes);
    }

    void prepare_nodes(unsigned int count) noexcept {
        nodes.reserve(count);
        const auto border = 0.5f * NODE_SIZE;
        for (unsigned int i = 0; i < count; i++) {
            const auto x = getRandomUniformFloat(border, world_size[0] - border);
            const auto y = getRandomUniformFloat(border, world_size[1] - border);
            const auto pos = Vec<2>{ x, y };
            constexpr auto MAX_VEL = 1.0f; 
            const auto vx = getRandomUniformFloat(0.0f, MAX_VEL);
            const auto vy = getRandomUniformFloat(0.0f, MAX_VEL);
            const auto vel = Vec<2>{ vx, vy };
            nodes.emplace_back(pos, vel);
        }

        glGenVertexArrays(1, &vao_nodes);
        glBindVertexArray(vao_nodes);
        {
            glGenBuffers(1, &vbo_nodes);
            glBindBuffer(GL_ARRAY_BUFFER, vbo_nodes);
            const unsigned int size_bytes = nodes.size() * vertices_per_node * node_vertex_components * sizeof(float);
            const float* data = 0;
            glBufferData(GL_ARRAY_BUFFER, size_bytes, data, GL_DYNAMIC_DRAW);

            specify_attribs_for_nodes(); // proper GL_ARRAY_BUFFER must be bound!
        }

        fill_nodes_indices_for_capacity(ibo_nodes, nodes.size());

        glBindVertexArray(0);
    }

    // std::optional<unsigned int> index_of_node_closest_to(const Vec<2>& pos) const noexcept {
    //     constexpr float threshold = 6.0f;
    //     int min_i = -1;
    //     float min_dst = std::numeric_limits<float>::max();
    //     for (unsigned int i = 0; i < nodes.size(); i++) {
    //         const auto dst = (nodes[i].position - pos).length_sqr();
    //         if (dst < min_dst && dst < threshold) {
    //             min_dst = dst;
    //             min_i = i;
    //         }
    //     }
    //     if (min_i >= 0) {
    //         return { static_cast<unsigned int>(min_i) };
    //     }
    //     return std::nullopt;
    // }

    // unsigned int index_of_node_with_id(unsigned int id) const noexcept {
    //     for (unsigned int i = 0; i < nodes.size(); i++) {
    //         if (nodes[i].id == id) return i;
    //     }
    //     std::cout << "[ASSERTION]: no Node with id = " << id << '\n';
    //     assert(false);
    // }

    void specify_attribs_for_nodes() const noexcept {
        {
            const auto index = 0;
            glEnableVertexAttribArray(index);
            const auto floats_per_vertex = 2; // inner_xy
            const auto stride_bytes = node_vertex_components * sizeof(float);
            const auto offset_bytes = 0;
            glVertexAttribPointer(index, floats_per_vertex, GL_FLOAT, GL_FALSE, stride_bytes, reinterpret_cast<const void*>(offset_bytes));
        }
        {
            const auto index = 1;
            glEnableVertexAttribArray(index);
            const auto floats_per_vertex = 3; // x, y, z
            const auto stride_bytes = node_vertex_components * sizeof(float);
            const auto offset_bytes = 2 * sizeof(float); // skip inner_xy
            glVertexAttribPointer(index, floats_per_vertex, GL_FLOAT, GL_FALSE, stride_bytes, reinterpret_cast<const void*>(offset_bytes));
        }
        {
            const auto index = 2;
            glEnableVertexAttribArray(index);
            const auto floats_per_vertex = 3; // color3
            const auto stride_bytes = node_vertex_components * sizeof(float);
            const auto offset_bytes = (2 + 3) * sizeof(float); // skip inner_xy and x,y,z
            glVertexAttribPointer(index, floats_per_vertex, GL_FLOAT, GL_FALSE, stride_bytes, reinterpret_cast<const void*>(offset_bytes));
        }
    }

    void update_and_render(float dt) noexcept {
        do_physics(dt);
        resubmit_nodes_vertices();
        render_nodes();
    }

    void render_nodes() const noexcept {
        shader_node.bind();
        glBindVertexArray(vao_nodes);
        glDrawElements(GL_TRIANGLES, nodes.size() * indices_per_node, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }

    void do_physics(float dt) noexcept {
        constexpr float speed_scaler = 5.0f * 0.0000025f;
        static const auto bound = [this](Node& node){
            static constexpr auto border = 0.5f * NODE_SIZE;
            if ((node.pos[0] < border) || ((world_size[0] - border) < node.pos[0])) node.vel[0] = -node.vel[0];
            if ((node.pos[1] < border) || ((world_size[1] - border) < node.pos[1])) node.vel[1] = -node.vel[1];
        };

        for (unsigned int i = 0; i < nodes.size(); i++) {
            auto& node = nodes[i];
            node.pos += node.vel * (speed_scaler * dt);
            bound(node);
        }
    }

    // TODO @speed store this ram-buffer separately for easier modification
    void resubmit_nodes_vertices() const noexcept {
        glBindBuffer(GL_ARRAY_BUFFER, vbo_nodes);
        const auto count = nodes.size() * vertices_per_node * node_vertex_components;
        const auto actual_size_bytes = count * sizeof(float);
        float data[count];

        for (unsigned int i = 0; i < nodes.size(); i++) {
            const auto color = Vec<3>{ 0.8f, 0.2f, 0.4f };
            const auto& node = nodes[i];
            const auto half_size = 0.5f * NODE_SIZE;
            const float z = z_nodes + z_delta * i;

            data[i * vertices_per_node * node_vertex_components + 0 * node_vertex_components + 0] = -1.0f;
            data[i * vertices_per_node * node_vertex_components + 0 * node_vertex_components + 1] = -1.0f;
            data[i * vertices_per_node * node_vertex_components + 0 * node_vertex_components + 2] = node.pos[0] - half_size;
            data[i * vertices_per_node * node_vertex_components + 0 * node_vertex_components + 3] = node.pos[1] - half_size;
            data[i * vertices_per_node * node_vertex_components + 0 * node_vertex_components + 4] = z;
            data[i * vertices_per_node * node_vertex_components + 0 * node_vertex_components + 5] = color[0];
            data[i * vertices_per_node * node_vertex_components + 0 * node_vertex_components + 6] = color[1];
            data[i * vertices_per_node * node_vertex_components + 0 * node_vertex_components + 7] = color[2];

            data[i * vertices_per_node * node_vertex_components + 1 * node_vertex_components + 0] = -1.0f;
            data[i * vertices_per_node * node_vertex_components + 1 * node_vertex_components + 1] = +1.0f;
            data[i * vertices_per_node * node_vertex_components + 1 * node_vertex_components + 2] = node.pos[0] - half_size;
            data[i * vertices_per_node * node_vertex_components + 1 * node_vertex_components + 3] = node.pos[1] + half_size;
            data[i * vertices_per_node * node_vertex_components + 1 * node_vertex_components + 4] = z;
            data[i * vertices_per_node * node_vertex_components + 1 * node_vertex_components + 5] = color[0];
            data[i * vertices_per_node * node_vertex_components + 1 * node_vertex_components + 6] = color[1];
            data[i * vertices_per_node * node_vertex_components + 1 * node_vertex_components + 7] = color[2];

            data[i * vertices_per_node * node_vertex_components + 2 * node_vertex_components + 0] = +1.0f;
            data[i * vertices_per_node * node_vertex_components + 2 * node_vertex_components + 1] = +1.0f;
            data[i * vertices_per_node * node_vertex_components + 2 * node_vertex_components + 2] = node.pos[0] + half_size;
            data[i * vertices_per_node * node_vertex_components + 2 * node_vertex_components + 3] = node.pos[1] + half_size;
            data[i * vertices_per_node * node_vertex_components + 2 * node_vertex_components + 4] = z;
            data[i * vertices_per_node * node_vertex_components + 2 * node_vertex_components + 5] = color[0];
            data[i * vertices_per_node * node_vertex_components + 2 * node_vertex_components + 6] = color[1];
            data[i * vertices_per_node * node_vertex_components + 2 * node_vertex_components + 7] = color[2];

            data[i * vertices_per_node * node_vertex_components + 3 * node_vertex_components + 0] = +1.0f;
            data[i * vertices_per_node * node_vertex_components + 3 * node_vertex_components + 1] = -1.0f;
            data[i * vertices_per_node * node_vertex_components + 3 * node_vertex_components + 2] = node.pos[0] + half_size;
            data[i * vertices_per_node * node_vertex_components + 3 * node_vertex_components + 3] = node.pos[1] - half_size;
            data[i * vertices_per_node * node_vertex_components + 3 * node_vertex_components + 4] = z;
            data[i * vertices_per_node * node_vertex_components + 3 * node_vertex_components + 5] = color[0];
            data[i * vertices_per_node * node_vertex_components + 3 * node_vertex_components + 6] = color[1];
            data[i * vertices_per_node * node_vertex_components + 3 * node_vertex_components + 7] = color[2];
        }

        glBufferSubData(GL_ARRAY_BUFFER, 0, actual_size_bytes, data);
    }

    // Can be done in advance for capacity, because indices are the same for all Nodes
    static void fill_nodes_indices_for_capacity(unsigned int& ibo_nodes, unsigned int nodes_capacity) noexcept {
        unsigned int data[indices_per_node * nodes_capacity];
        for (unsigned int i = 0; i < nodes_capacity; i++) {
            data[i * indices_per_node + 0] = i * vertices_per_node + 0;
            data[i * indices_per_node + 1] = i * vertices_per_node + 1;
            data[i * indices_per_node + 2] = i * vertices_per_node + 2;
            data[i * indices_per_node + 3] = i * vertices_per_node + 0;
            data[i * indices_per_node + 4] = i * vertices_per_node + 2;
            data[i * indices_per_node + 5] = i * vertices_per_node + 3;
        }
        const unsigned int size_bytes = indices_per_node * nodes_capacity * sizeof(data[0]);

        glGenBuffers(1, &ibo_nodes);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_nodes);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, size_bytes, data, GL_STATIC_DRAW); // will be changed a couple of times
    }

    void set_mvp(const Matrix4f& mvp) noexcept {
        shader_node.bind();
        shader_node.setUniformMat4f("u_mvp", mvp);
    }

    void set_size(const Vec<2>& size) noexcept {
        world_size = size;
    }
};




struct Game {
    private:
        bool game_should_close = false;

        static constexpr float world_size_y = 100.0f;
        Vec<2> window_size;
        Vec<2> world_size;
        Matrix4f mvp;
        World world;

        bool pressed_s = false;
        bool pressed_r = false;
        bool pressed_lmb = false;
        bool pressed_rmb = false;
        bool pressed_mmb = false;
        bool pressed_space = false;
        bool physics_on = false;

        static Matrix4f mvp_for_world_size(const Vec<2>& world_size) noexcept {
            return Matrix4f::orthographic(0.0f, world_size[0], 0.0f, world_size[1], -1.0f, 1.0f);
        }

        static Vec<2> world_size_for_aspect(float aspect_w_h) noexcept {
            return Vec<2>{ world_size_y * aspect_w_h, world_size_y };
        }

        Vec<2> cursor_to_world_coord(const Vec<2>& cursor) const noexcept {
            return Vec<2>{ cursor[0] / window_size[0] * world_size[0], world_size[1] - cursor[1] / window_size[1] * world_size[1] };
        }

        static Vec<2> get_cursor(GLFWwindow* window) noexcept {
            double xpos, ypos;
            glfwGetCursorPos(window, &xpos, &ypos);
            return Vec<2>{ static_cast<float>(xpos), static_cast<float>(ypos) };
        }

    public:
        Game(unsigned int width, unsigned int height) noexcept
                : window_size(Vec<2>{ 1.0f * width, 1.0f * height }),
                  world_size(world_size_for_aspect(static_cast<float>(width) / static_cast<float>(height))),
                  mvp(mvp_for_world_size(world_size)),
                  world(World{ world_size }) {
            world.set_mvp(mvp);
        }

        void update_and_render(float dt) noexcept {
            world.update_and_render(dt);
        }

        void register_input(GLFWwindow* window) noexcept {
            if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) game_should_close = true;

            // if (!pressed_space && (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)) {
            //     pressed_space = true;
            //     if (physics_on) {
            //         physics_on = false;
            //         world.turn_physics_off();
            //     } else {
            //         physics_on = true;
            //         world.turn_physics_on();
            //     }
            // }
            // if (pressed_space && (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_RELEASE)) {
            //     pressed_space = false;
            // }

            // static std::optional<Vec<2>> memorized_coord = std::nullopt;
            // if (!pressed_lmb && glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
            //     pressed_lmb = true;
            //
            //     const auto coord = cursor_to_world_coord(get_cursor(window));
            //     if (world.index_of_node_closest_to(coord)) {
            //         memorized_coord = { coord };
            //     } else {
            //         world.spawn_node(coord);
            //         memorized_coord = std::nullopt;
            //     }
            // }
            // if (pressed_lmb && glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_RELEASE) {
            //     pressed_lmb = false;
            //
            //     if (memorized_coord) {
            //         const auto coord2 = cursor_to_world_coord(get_cursor(window));
            //         world.spawn_line(*memorized_coord, coord2);
            //     }
            // }
        }

        void register_mouse(float x, float y) noexcept {}

        void reset_dimensions(unsigned int width, unsigned int height) noexcept {
            window_size = Vec<2>{ 1.0f * width, 1.0f * height };
            const float aspect_w_h = static_cast<float>(width) / static_cast<float>(height);
            world_size = world_size_for_aspect(aspect_w_h);
            mvp = mvp_for_world_size(world_size);
            world.set_mvp(mvp);
            world.set_size(world_size);
        }

        bool should_close() const noexcept {
            return game_should_close;
        }
};


#endif
