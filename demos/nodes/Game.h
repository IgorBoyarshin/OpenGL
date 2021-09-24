#ifndef GAME_H
#define GAME_H

#include "Shader.h"
#include "Texture.h"
#include "Vector2f.h"

#include <cmath>
#include <algorithm>
#include <random>

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
    const float r = getRandomUniformFloat(0.1f, 1.0f);
    const float g = getRandomUniformFloat(0.1f, 1.0f);
    const float b = getRandomUniformFloat(0.1f, 1.0f);
    return Vector3f{ r, g, b };
}


static constexpr unsigned int ID_MAX = 0xFFFFFFF;


static constexpr float NODE_SIZE = 8.0f;
struct Node {
    unsigned int id;
    Vector2f position;
    Vector3f color;

    Node(Vector2f&& position) :
        id(getRandomUniformInt(static_cast<unsigned int>(0), ID_MAX)),
        position(std::move(position)),
        color(random_color()) {}

    Node(unsigned int id, Vector2f&& position) :
        id(id),
        position(std::move(position)),
        color(random_color()) {}
};


struct Line {
    unsigned int id1, id2;

    Line(unsigned int id1, unsigned int id2) : id1(id1), id2(id2) {}
};


struct World {
    // const float size_x, size_y;

    std::vector<Node> nodes;
    // For Lines, assume every Line has unique Dots; this gives us a maximum
    // Dots count possible of twice the Lines amount.
    std::vector<Line> lines;

    unsigned int vao_nodes;
    unsigned int vbo_nodes;
    unsigned int ibo_nodes;

    unsigned int vao_lines;
    unsigned int vbo_lines;
    unsigned int ibo_lines;

    unsigned int vao, vbo, ibo;

    static constexpr unsigned int node_vertex_components = 2 + 3 + 3; // inner_xy + x,y,z + color3
    static constexpr unsigned int indices_per_node = 6;
    static constexpr unsigned int vertices_per_node = 4;
    unsigned int last_known_nodes_capacity;
    // bool nodes_changed = false;

    static constexpr unsigned int dot_vertex_components = 3 + 3; // x,y,z + color3
    static constexpr auto indices_per_line = 2;
    static constexpr unsigned int vertices_per_dot = 2;
    unsigned int last_known_lines_capacity;
    // bool lines_changed = false;

    Shader shader_node{"node.vert", "node.frag"};
    Shader shader_line{"line.vert", "line.frag"};
    static constexpr float z_nodes = 0.3f;
    static constexpr float z_lines = 0.2f;

    static constexpr float z_delta = 0.00001f;

    World() {
        glLineWidth(5.0f);
        const auto lines_starting_capacity = 16;
        const auto nodes_starting_capacity = 16;
        prepare_nodes(nodes_starting_capacity);
        prepare_lines(nodes_starting_capacity, lines_starting_capacity);
        add_sample_grid();
    }

    ~World() {
        glDeleteBuffers(1, &ibo_nodes);
        glDeleteBuffers(1, &vbo_nodes);
        glDeleteVertexArrays(1, &vao_nodes);
        glDeleteBuffers(1, &ibo_lines);
        glDeleteBuffers(1, &vbo_lines);
        glDeleteVertexArrays(1, &vao_lines);
    }

    void prepare_nodes(unsigned int starting_capacity) {
        nodes.reserve(starting_capacity);
        last_known_nodes_capacity = nodes.capacity();

        glGenVertexArrays(1, &vao_nodes);
        glBindVertexArray(vao_nodes);
        {
            glGenBuffers(1, &vbo_nodes);
            glBindBuffer(GL_ARRAY_BUFFER, vbo_nodes);
            const unsigned int size_bytes = nodes.capacity() * vertices_per_node * node_vertex_components * sizeof(float);
            const float* data = 0;
            glBufferData(GL_ARRAY_BUFFER, size_bytes, data, GL_DYNAMIC_DRAW);

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

        fill_nodes_indices_for_capacity(ibo_nodes, nodes.capacity());

        glBindVertexArray(0);
    }

    // Nodes count is for Lines' VBO,
    // Lines count is for Lines' IBO.
    void prepare_lines(unsigned int nodes_capacity, unsigned int lines_starting_capacity) {
        lines.reserve(lines_starting_capacity);
        last_known_lines_capacity = lines.capacity();

        glGenVertexArrays(1, &vao_lines);
        glBindVertexArray(vao_lines);
        {
            // Stores Dots
            glGenBuffers(1, &vbo_lines);
            glBindBuffer(GL_ARRAY_BUFFER, vbo_lines);
            const unsigned int size_bytes = nodes_capacity * dot_vertex_components * sizeof(float);
            const float* data = 0;
            glBufferData(GL_ARRAY_BUFFER, size_bytes, data, GL_DYNAMIC_DRAW);

            {
                const auto index = 0;
                glEnableVertexAttribArray(index);
                const auto floats_per_vertex = 3; // x, y, z
                const auto stride_bytes = dot_vertex_components * sizeof(float);
                const auto offset_bytes = 0;
                glVertexAttribPointer(index, floats_per_vertex, GL_FLOAT, GL_FALSE, stride_bytes, reinterpret_cast<const void*>(offset_bytes));
            }
            {
                const auto index = 1;
                glEnableVertexAttribArray(index);
                const auto floats_per_vertex = 3; // color3
                const auto stride_bytes = dot_vertex_components * sizeof(float);
                const auto offset_bytes = 3 * sizeof(float); // skip x,y,z
                glVertexAttribPointer(index, floats_per_vertex, GL_FLOAT, GL_FALSE, stride_bytes, reinterpret_cast<const void*>(offset_bytes));
            }
        }

        // The filling will be done upon Line addition
        allocate_lines_indices_for_capacity(ibo_lines, lines.capacity());

        glBindVertexArray(0);
    }

    void add_sample_grid() noexcept {
        add_node(Node{ 1, Vector2f{10.0f, 20.0f}});
        add_node(Node{ 2, Vector2f{20.0f, 20.0f}});
        add_node(Node{ 3, Vector2f{30.0f, 30.0f}});
        // Depends on Nodes, so must be done after the Nodes have been set up
        add_line(Line{ 1, 2 });
        add_line(Line{ 2, 3 });
    }

    void update_and_render(float dt) noexcept {
        do_physics(dt);
        render_lines();
        render_nodes();
    }

    void render_nodes() const noexcept {
        shader_node.bind();
        glBindVertexArray(vao_nodes);
        glDrawElements(GL_TRIANGLES, nodes.size() * indices_per_node, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }

    void render_lines() const noexcept {
        shader_line.bind();
        glBindVertexArray(vao_lines);
        glDrawElements(GL_LINES, lines.size() * indices_per_line, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }

    void do_physics(float dt) noexcept {

    }

    // TODO @speed add nodes as a batch
    void add_node(const Node& node) noexcept {
        nodes.push_back(node);

        if (last_known_nodes_capacity != nodes.capacity()) {
            std::cout << "Capacity of nodes change" << '\n';
            last_known_nodes_capacity = nodes.capacity();

            glBindVertexArray(vao_nodes);
            {
                // Reallocate IBO
                glDeleteBuffers(1, &ibo_nodes);
                fill_nodes_indices_for_capacity(ibo_nodes, nodes.capacity());

                // Reallocate VBO
                glDeleteBuffers(1, &vbo_nodes);
                glGenBuffers(1, &vbo_nodes);
                glBindBuffer(GL_ARRAY_BUFFER, vbo_nodes);
                const unsigned int size_bytes = nodes.capacity() * vertices_per_node * node_vertex_components * sizeof(float);
                const float* data = 0;
                glBufferData(GL_ARRAY_BUFFER, size_bytes, data, GL_DYNAMIC_DRAW);
                // Refill the old part of VBO
                resubmit_nodes_vertices();
            }

            glBindVertexArray(vao_lines);
            {
                // Reallocate VBO
                glDeleteBuffers(1, &vbo_lines);
                glGenBuffers(1, &vbo_lines);
                glBindBuffer(GL_ARRAY_BUFFER, vbo_lines);
                const unsigned int size_bytes = nodes.capacity() * vertices_per_dot * dot_vertex_components * sizeof(float);
                const float* data = 0;
                glBufferData(GL_ARRAY_BUFFER, size_bytes, data, GL_DYNAMIC_DRAW);
                // Refill the old part of VBO
                resubmit_lines_vertices();
            }
            glBindVertexArray(0);
        }


        // Append the Node part
        {
            glBindBuffer(GL_ARRAY_BUFFER, vbo_nodes);
            const unsigned int skip_size_bytes = (nodes.size() - 1) * vertices_per_node * node_vertex_components * sizeof(float);
            const float half_size = 0.5f * NODE_SIZE;
            const float z = z_nodes + z_delta * (nodes.size() - 1);
            const float data[vertices_per_node * node_vertex_components] = {
                -1.0f, -1.0f, node.position.x - half_size, node.position.y - half_size, z, node.color.x, node.color.y, node.color.z,
                -1.0f, +1.0f, node.position.x - half_size, node.position.y + half_size, z, node.color.x, node.color.y, node.color.z,
                +1.0f, +1.0f, node.position.x + half_size, node.position.y + half_size, z, node.color.x, node.color.y, node.color.z,
                +1.0f, -1.0f, node.position.x + half_size, node.position.y - half_size, z, node.color.x, node.color.y, node.color.z
            };
            const unsigned int data_size_bytes = vertices_per_node * node_vertex_components * sizeof(data[0]);
            glBufferSubData(GL_ARRAY_BUFFER, skip_size_bytes, data_size_bytes, data);
        }

        // Append the Dot part
        {
            glBindBuffer(GL_ARRAY_BUFFER, vbo_lines);
            const unsigned int skip_size_bytes = (nodes.size() - 1) * dot_vertex_components * sizeof(float);
            const float z = z_lines + z_delta * (nodes.size() - 1);
            const float data[dot_vertex_components] = {
                node.position.x, node.position.y, z, node.color.x, node.color.y, node.color.z
            };
            const unsigned int data_size_bytes = dot_vertex_components * sizeof(data[0]);
            glBufferSubData(GL_ARRAY_BUFFER, skip_size_bytes, data_size_bytes, data);
        }
    }

    // TODO @speed add lines as a batch
    void add_line(const Line& line) noexcept {
        lines.push_back(line);

        if (last_known_lines_capacity != lines.capacity()) {
            std::cout << "Capacity of lines change" << '\n';
            last_known_lines_capacity = lines.capacity();

            glBindVertexArray(vao_lines);
            glDeleteBuffers(1, &ibo_lines);
            allocate_lines_indices_for_capacity(ibo_lines, lines.capacity());
            glBindVertexArray(0);
        }

        fill_lines_indices(ibo_lines, lines);
    }

    // TODO @speed store this ram-buffer separately for easier modification
    void resubmit_nodes_vertices() const noexcept {
        glBindBuffer(GL_ARRAY_BUFFER, vbo_nodes);
        const auto count = nodes.size() * vertices_per_node * node_vertex_components;
        const auto actual_size_bytes = count * sizeof(float);
        float data[count];

        for (unsigned int i = 0; i < nodes.size(); i++) {
            const auto& node = nodes[i];
            const auto half_size = 0.5f * NODE_SIZE;
            const float z = z_nodes + z_delta * i;

            data[i * vertices_per_node * node_vertex_components + 0 * node_vertex_components + 0] = -1.0f;
            data[i * vertices_per_node * node_vertex_components + 0 * node_vertex_components + 1] = -1.0f;
            data[i * vertices_per_node * node_vertex_components + 0 * node_vertex_components + 2] = node.position.x - half_size;
            data[i * vertices_per_node * node_vertex_components + 0 * node_vertex_components + 3] = node.position.y - half_size;
            data[i * vertices_per_node * node_vertex_components + 0 * node_vertex_components + 4] = z;
            data[i * vertices_per_node * node_vertex_components + 0 * node_vertex_components + 5] = node.color.x;
            data[i * vertices_per_node * node_vertex_components + 0 * node_vertex_components + 6] = node.color.y;
            data[i * vertices_per_node * node_vertex_components + 0 * node_vertex_components + 7] = node.color.z;

            data[i * vertices_per_node * node_vertex_components + 1 * node_vertex_components + 0] = -1.0f;
            data[i * vertices_per_node * node_vertex_components + 1 * node_vertex_components + 1] = +1.0f;
            data[i * vertices_per_node * node_vertex_components + 1 * node_vertex_components + 2] = node.position.x - half_size;
            data[i * vertices_per_node * node_vertex_components + 1 * node_vertex_components + 3] = node.position.y + half_size;
            data[i * vertices_per_node * node_vertex_components + 1 * node_vertex_components + 4] = z;
            data[i * vertices_per_node * node_vertex_components + 1 * node_vertex_components + 5] = node.color.x;
            data[i * vertices_per_node * node_vertex_components + 1 * node_vertex_components + 6] = node.color.y;
            data[i * vertices_per_node * node_vertex_components + 1 * node_vertex_components + 7] = node.color.z;

            data[i * vertices_per_node * node_vertex_components + 2 * node_vertex_components + 0] = +1.0f;
            data[i * vertices_per_node * node_vertex_components + 2 * node_vertex_components + 1] = +1.0f;
            data[i * vertices_per_node * node_vertex_components + 2 * node_vertex_components + 2] = node.position.x + half_size;
            data[i * vertices_per_node * node_vertex_components + 2 * node_vertex_components + 3] = node.position.y + half_size;
            data[i * vertices_per_node * node_vertex_components + 2 * node_vertex_components + 4] = z;
            data[i * vertices_per_node * node_vertex_components + 2 * node_vertex_components + 5] = node.color.x;
            data[i * vertices_per_node * node_vertex_components + 2 * node_vertex_components + 6] = node.color.y;
            data[i * vertices_per_node * node_vertex_components + 2 * node_vertex_components + 7] = node.color.z;

            data[i * vertices_per_node * node_vertex_components + 3 * node_vertex_components + 0] = +1.0f;
            data[i * vertices_per_node * node_vertex_components + 3 * node_vertex_components + 1] = -1.0f;
            data[i * vertices_per_node * node_vertex_components + 3 * node_vertex_components + 2] = node.position.x + half_size;
            data[i * vertices_per_node * node_vertex_components + 3 * node_vertex_components + 3] = node.position.y - half_size;
            data[i * vertices_per_node * node_vertex_components + 3 * node_vertex_components + 4] = z;
            data[i * vertices_per_node * node_vertex_components + 3 * node_vertex_components + 5] = node.color.x;
            data[i * vertices_per_node * node_vertex_components + 3 * node_vertex_components + 6] = node.color.y;
            data[i * vertices_per_node * node_vertex_components + 3 * node_vertex_components + 7] = node.color.z;
        }

        glBufferSubData(GL_ARRAY_BUFFER, 0, actual_size_bytes, data);
    }

    // Have a Dot in the center of every Node
    void resubmit_lines_vertices() const noexcept {
        glBindBuffer(GL_ARRAY_BUFFER, vbo_lines);

        const auto count = nodes.size() * dot_vertex_components;
        const auto actual_size_bytes = count * sizeof(float);
        float data[count];

        for (unsigned int i = 0; i < nodes.size(); i++) {
            const auto& node = nodes[i];
            const float z = z_lines + z_delta * i;

            data[i * dot_vertex_components + 0] = node.position.x;
            data[i * dot_vertex_components + 1] = node.position.y;
            data[i * dot_vertex_components + 2] = z;
            data[i * dot_vertex_components + 3] = node.color.x;
            data[i * dot_vertex_components + 4] = node.color.y;
            data[i * dot_vertex_components + 5] = node.color.z;
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

    // Cannot be done in advance for capacity, because indices are different,
    // so must be called on every Line addition/removal, so based on size.
    static void allocate_lines_indices_for_capacity(unsigned int& ibo_lines, unsigned int lines_capacity) noexcept {
        const unsigned int* data = 0;
        const unsigned int size_bytes = indices_per_line * lines_capacity * sizeof(unsigned int);

        glGenBuffers(1, &ibo_lines);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_lines);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, size_bytes, data, GL_DYNAMIC_DRAW); // will be changed on every Line addition/removal
    }

    void fill_lines_indices(unsigned int ibo_lines, const std::vector<Line>& lines) const noexcept {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_lines);

        static const auto index_of_node_with_id = [this](unsigned int id){
            for (unsigned int i = 0; i < nodes.size(); i++) {
                if (nodes[i].id == id) return i;
            }
            std::cout << "[ASSERTION]: no Node with id = " << id << '\n';
            assert(false);
        };
        const auto count = lines.size() * indices_per_line;
        unsigned int data[count];
        for (unsigned int i = 0; i < lines.size(); i++) {
            data[i * indices_per_line + 0] = index_of_node_with_id(lines[i].id1);
            data[i * indices_per_line + 1] = index_of_node_with_id(lines[i].id2);
        }

        const auto skip_size_bytes = 0;
        const auto data_size_bytes = count * sizeof(data[0]);
        glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, skip_size_bytes, data_size_bytes, data);
    }

    void set_mvp(const Matrix4f& mvp) noexcept {
        shader_node.bind();
        shader_node.setUniformMat4f("u_mvp", mvp);
        shader_line.bind();
        shader_line.setUniformMat4f("u_mvp", mvp);
    }
};



using InputType = unsigned int;
enum Input : InputType {
    MOVE_UP    = 1 << 1,
    MOVE_DOWN  = 1 << 2,
    MOVE_LEFT  = 1 << 3,
    MOVE_RIGHT = 1 << 4
};


struct Game {
    private:
        bool game_should_close = false;
        InputType input;

        World world;
        // static constexpr unsigned int map_texture_id = 0;
        // static constexpr float map_z      = 0.0f;
        // static constexpr float entities_z = 0.3f;
        Matrix4f mvp;
        static constexpr float world_size_y = 100.0f;
        Vector2f world_size;


        void process_input(float dt) noexcept {
            int delta_x = 0;
            int delta_y = 0;
            if (input & Input::MOVE_LEFT ) delta_x -= 1;
            if (input & Input::MOVE_RIGHT) delta_x += 1;
            if (input & Input::MOVE_UP   ) delta_y += 1;
            if (input & Input::MOVE_DOWN ) delta_y -= 1;

            // const int magnitude = std::abs(delta_x) + std::abs(delta_y);
            // if (magnitude > 0) {
            //     static constexpr float one_over_sqrt_2 = 1.0f / 1.41421356f;
            //     const float step = dt * ((magnitude == 2) ? one_over_sqrt_2 : 1.0f);
            //     const Vector2f direction{ delta_x * step, delta_y * step };
            //     world.move_first_entity_in_direction(direction);
            // }

            input = 0; // reset
        }

        static Matrix4f mvp_for_world_size(const Vector2f& world_size) noexcept {
            return Matrix4f::orthographic(0.0f, world_size.x, 0.0f, world_size.y, -1.0f, 1.0f);
        }

        static Vector2f world_size_for_aspect(float aspect_w_h) noexcept {
            return Vector2f{ world_size_y * aspect_w_h, world_size_y };
        }

        // static Vector2f camera_centered_on(const Vector2f& position, const Vector2f& world_size) noexcept {
        //     return position - world_size / 2.0f;
        // }

    public:
        Game(unsigned int width, unsigned int height) noexcept {
            const float aspect_w_h = static_cast<float>(width) / static_cast<float>(height);
            world_size = world_size_for_aspect(aspect_w_h);
            mvp = mvp_for_world_size(world_size);
            world.set_mvp(mvp);
            // world.set_frame_size(world_size);
        }

        // Game(unsigned int width, unsigned int height) noexcept :
        //         world(World(entities_z, map_z, map_texture_id)) {
        //     const float aspect_w_h = static_cast<float>(width) / static_cast<float>(height);
        //     world_size = world_size_for_aspect(aspect_w_h);
        //     mvp = mvp_for_world_size(world_size);
        //     world.set_mvp(mvp);
        //     world.set_frame_size(world_size);
        //
        //     const Vector2f position{ 1.0f, 1.0f };
        //     const Entity my_entity{.position = position, .size = 1.0f, .color = Vector3f{ 0.5f, 0.1f, 0.9f } };
        //     world.add_entity(my_entity); // our Entity will always be at index zero
        //     world.add_entity(Entity{.position = Vector2f{ 5.0f, 3.0f }, .size = 2.0f, .color = Vector3f{ 0.1f, 0.5f, 0.1f } });
        //     world.add_entity(Entity{.position = Vector2f{ 13.0f, 2.0f }, .size = 2.5f, .color = Vector3f{ 0.9f, 0.5f, 0.1f } });
        //     world.add_entity(Entity{.position = Vector2f{ 1.1f, 4.0f }, .size = 0.4f, .color = Vector3f{ 0.1f, 0.5f, 0.1f } });
        //     world.add_entity(Entity{.position = Vector2f{ 1.1f, 11.0f }, .size = 0.5f, .color = Vector3f{ 0.1f, 0.9f, 0.1f } });
        // }

        void update_and_render(float dt) noexcept {
            process_input(dt);
            world.update_and_render(dt);
        }

        void register_input(GLFWwindow* window) noexcept {
            if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) game_should_close = true;

            if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) input |= Input::MOVE_UP;
            if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) input |= Input::MOVE_DOWN;
            if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) input |= Input::MOVE_LEFT;
            if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) input |= Input::MOVE_RIGHT;
        }

        void register_mouse(float x, float y) noexcept {}

        void reset_dimensions(unsigned int width, unsigned int height) noexcept {
            const float aspect_w_h = static_cast<float>(width) / static_cast<float>(height);
            world_size = world_size_for_aspect(aspect_w_h);
            mvp = mvp_for_world_size(world_size);
            world.set_mvp(mvp);
            // world.set_frame_size(world_size);
        }

        bool should_close() const noexcept {
            return game_should_close;
        }
};


#endif
