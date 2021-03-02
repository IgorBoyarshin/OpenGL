#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <vector>
#include <array>
#include <random>
#include <numeric>
#include <algorithm>
#include <time.h>
#include <string_view>
#include <iomanip>

#include "Shader.h"
#include "Texture.h"
#include "Vector2f.h"


// float, double, long double
// [low, high)
template<typename T = float>
T getRandomUniformFloat(T low, T high) {
    static std::random_device rd;
    static std::mt19937 e2(rd());
    std::uniform_real_distribution<T> dist(low, high);

    return dist(e2);
}

struct GlobalState {
    bool resize_recalculation_required = false;
    unsigned int width = 1920;
    unsigned int height = 1080;
    unsigned int mouse_x = 0;
    unsigned int mouse_y = 0;
    bool pause = false;
};

static GlobalState global_state;


void APIENTRY openglCallbackFunction(
        __attribute__((unused)) GLenum source,
        GLenum type,
        GLuint id,
        GLenum severity,
        __attribute__((unused)) GLsizei length,
        const GLchar* message,
        __attribute__((unused)) const void* userParam
);

GLFWwindow* init(unsigned int widht, unsigned int height);
void processInput(GLFWwindow *window, float dt);




template<typename T>
struct Graph {
    private:
        std::vector<T> history;
        unsigned int history_i = 0;

        void debounce() noexcept {
            if (history_i >= history.size()) history_i = 0;
        }

        unsigned int vao;
        unsigned int ibo;
        unsigned int vbo;

        unsigned int size_background_bytes;
        unsigned int size_lines_bytes;

    public:
        Graph(unsigned int history_size) noexcept : history(std::vector<T>(history_size)) {
            // glLineWidth(1.0f);
            glGenVertexArrays(1, &vao);
            glBindVertexArray(vao);

            const auto size_background = 4 * (3 + 4); // x,y,z + color_vec4
            const float background_alpha = 0.8f;
            float background[size_background] = {
                // x, y, z, color_vec4
                0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, background_alpha,
                0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, background_alpha,
                1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, background_alpha,
                1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, background_alpha
            };
            size_background_bytes = size_background * sizeof(background[0]);

            const auto size_lines = history_size * (3 + 4); // x,y,z + color_vec4
            float lines[size_lines];
            for (unsigned int i = 0; i < history_size; i++) {
                lines[i * (3 + 4) + 0] = 0.0f; // x
                lines[i * (3 + 4) + 1] = 0.0f; // y
                lines[i * (3 + 4) + 2] = 0.1f; // z
                lines[i * (3 + 4) + 3] = 1.0f; // red
                lines[i * (3 + 4) + 4] = 1.0f; // green
                lines[i * (3 + 4) + 5] = 0.0f; // blue
                lines[i * (3 + 4) + 6] = 1.0f; // alpha
            }
            size_lines_bytes = size_lines * sizeof(lines[0]);

            glGenBuffers(1, &vbo);
            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            glBufferData(GL_ARRAY_BUFFER, size_background_bytes + size_lines_bytes, 0, GL_DYNAMIC_DRAW);
            glBufferSubData(GL_ARRAY_BUFFER, 0, size_background_bytes, background);
            glBufferSubData(GL_ARRAY_BUFFER, size_background_bytes, size_lines_bytes, lines);

            {
                glEnableVertexAttribArray(0);
                const auto floats_per_vertex = 3; // x, y, z
                const auto stride_bytes = (3 + 4) * sizeof(float);
                const auto offset = 0;
                glVertexAttribPointer(0, floats_per_vertex, GL_FLOAT, GL_FALSE, stride_bytes, reinterpret_cast<const void*>(offset));
            }

            {
                glEnableVertexAttribArray(1);
                const auto floats_per_vertex = 4; // color
                const auto stride_bytes = (3 + 4) * sizeof(float);
                const auto offset = 3 * sizeof(float); // skip x,y,z
                glVertexAttribPointer(1, floats_per_vertex, GL_FLOAT, GL_FALSE, stride_bytes, reinterpret_cast<const void*>(offset));
            }

            {
                const auto size = 6 + history_size;
                unsigned int indices[size];
                unsigned int i = 0;
                indices[i++] = 0;
                indices[i++] = 1;
                indices[i++] = 2;
                indices[i++] = 0;
                indices[i++] = 2;
                indices[i++] = 3;
                for (unsigned int j = 0; j < history_size; j++) indices[i + j] = 4 + j; // 4 == background vertices count
                const unsigned int size_bytes = size * sizeof(indices[0]);
                glGenBuffers(1, &ibo);
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
                glBufferData(GL_ELEMENT_ARRAY_BUFFER, size_bytes, indices, GL_STATIC_DRAW);
            }

            glBindVertexArray(0);
        }

        // NOTE: needs logic fix
        // void setHistorySize(unsigned int count) noexcept {
        //     // Cycle-shift to aligned state to make resizing valid
        //     // Note: performance can be improved
        //     while (history_i != 0) {
        //         T first = history.at(0);
        //         for (unsigned int i = 0; i < history.size() - 1; i++) history[i] = history[i + 1];
        //         history[history.size() - 1] = first;
        //         history_i--;
        //     }
        //
        //     history.resize(count);
        // }
        void pushValue(T value) noexcept {
            history[history_i++] = value;
            debounce();

            const auto size_lines = history.size() * (3 + 4); // x,y + color_vec4
            float lines[size_lines];
            const float step = 1.0f / history.size();
            const float max_y = *std::max_element(history.cbegin(), history.cend());

            unsigned int index = history_i;
            for (unsigned int i = 0; i < history.size(); i++) {
                if (index == history.size()) index = 0;
                lines[i * (3 + 4) + 0] = step * i; // x
                lines[i * (3 + 4) + 1] = history[index] / max_y; // y
                lines[i * (3 + 4) + 2] = 0.1f; // z
                // std::cout << "Height = " << lines[i * (3 + 4) + 1] << '\n';
                lines[i * (3 + 4) + 3] = 1.0f; // red
                lines[i * (3 + 4) + 4] = 1.0f; // green
                lines[i * (3 + 4) + 5] = 0.0f; // blue
                lines[i * (3 + 4) + 6] = 1.0f; // alpha
                index++;
            }
            // std::cout << "==========" << '\n';
            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            glBufferSubData(GL_ARRAY_BUFFER, size_background_bytes, size_lines_bytes, lines);

        }
        void reset() noexcept {
            history_i = 0;
        }

        void draw() const noexcept {
            glBindVertexArray(vao);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
            glDrawElements(GL_LINE_STRIP, history.size(), GL_UNSIGNED_INT, reinterpret_cast<const void*>(6 * sizeof(unsigned int))); // skip 2 triangles
        }
};





struct FieldSize {
    unsigned int width;
    unsigned int height;
    float border_x;
    float border_y;
    float tile_size;
    float border;
    inline FieldSize(unsigned int tile, unsigned int border_size,
            unsigned int window_width, unsigned int window_height) noexcept
            : tile_size(static_cast<float>(tile)), border(static_cast<float>(border_size)) {
        const auto unit = tile + border_size;
        width = (window_width - border_size) / unit;
        height = (window_height - border_size) / unit;
        border_x = (window_width - border_size - width * unit) / 2.0f;
        border_y = (window_height - border_size - height * unit) / 2.0f;
    }
};



struct FieldData {
    unsigned int vao;
    unsigned int vbo_positions;
    unsigned int vbo_colors;
    unsigned int ibo;
    unsigned int indices_count;

    void destroy() const noexcept {
        glDeleteBuffers(1, &vbo_positions);
        glDeleteBuffers(1, &vbo_colors);
        glDeleteBuffers(1, &ibo);
        glDeleteVertexArrays(1, &vao);
    }
};


FieldData constructFieldData(std::vector<float>&& positions, std::vector<unsigned int>&& indices, const std::vector<float>& colors) noexcept {
    unsigned int vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    unsigned int vbo_positions;
    {
        const unsigned int size_bytes = positions.size() * sizeof(positions[0]);
        glGenBuffers(1, &vbo_positions);
        glBindBuffer(GL_ARRAY_BUFFER, vbo_positions);
        glBufferData(GL_ARRAY_BUFFER, size_bytes, positions.data(), GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        const auto floats_per_vertex = 2; // x, y
        const auto stride_bytes = floats_per_vertex * sizeof(float);
        const auto offset = 0;
        glVertexAttribPointer(0, floats_per_vertex, GL_FLOAT, GL_FALSE, stride_bytes, reinterpret_cast<const void*>(offset));
    }

    unsigned int vbo_colors;
    {
        const unsigned int size_bytes = colors.size() * sizeof(colors[0]);
        glGenBuffers(1, &vbo_colors);
        glBindBuffer(GL_ARRAY_BUFFER, vbo_colors);
        glBufferData(GL_ARRAY_BUFFER, size_bytes, colors.data(), GL_DYNAMIC_DRAW);

        glEnableVertexAttribArray(1);
        const auto floats_per_vertex = 3; // color
        const auto stride_bytes = floats_per_vertex * sizeof(float);
        const auto offset = 0;
        glVertexAttribPointer(1, floats_per_vertex, GL_FLOAT, GL_FALSE, stride_bytes, reinterpret_cast<const void*>(offset));
    }

    unsigned int ibo;
    {
        const unsigned int size_bytes = indices.size() * sizeof(indices[0]);
        glGenBuffers(1, &ibo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, size_bytes, indices.data(), GL_STATIC_DRAW);
    }

    glBindVertexArray(0);

    return FieldData{ .vao = vao, .vbo_positions = vbo_positions, .vbo_colors = vbo_colors,
        .ibo = ibo, .indices_count = static_cast<unsigned int>(indices.size()) };
}


FieldData constructField(const FieldSize& field_size, const std::vector<float>& colors) noexcept {
    const float unit = field_size.border + field_size.tile_size;

    std::vector<float> positions;
    positions.reserve(2 * 4 * field_size.width * field_size.height);
    std::vector<unsigned int> indices;
    indices.reserve(6 * field_size.width * field_size.height);

    for (unsigned int column = 0; column < field_size.width; column++) {
        for (unsigned int row = 0; row < field_size.height; row++) {
            const Vector2f lower_left {
                field_size.border_x + unit * column + field_size.border,
                field_size.border_y + unit * row    + field_size.border
            };
            const Vector2f lower_right = lower_left + Vector2f{ field_size.tile_size, 0.0f };
            const Vector2f upper_left = lower_left + Vector2f{ 0.0f, field_size.tile_size };
            const Vector2f upper_right = lower_left + Vector2f{ field_size.tile_size, field_size.tile_size };

            positions.push_back(lower_left.x);
            positions.push_back(lower_left.y);
            positions.push_back(upper_left.x);
            positions.push_back(upper_left.y);
            positions.push_back(upper_right.x);
            positions.push_back(upper_right.y);
            positions.push_back(lower_right.x);
            positions.push_back(lower_right.y);

            const unsigned int offset = (column * field_size.height + row) * 4;
            indices.push_back(offset + 0);
            indices.push_back(offset + 1);
            indices.push_back(offset + 2);
            indices.push_back(offset + 0);
            indices.push_back(offset + 2);
            indices.push_back(offset + 3);
        }
    }

    return constructFieldData(std::move(positions), std::move(indices), colors);
}


void regenerateColors(std::vector<float>& colors) noexcept {
    const auto unit_len = 3 * 4;
    for (unsigned int i = 0; i < colors.size() / unit_len; i++) {
        const Vector3f color{ getRandomUniformFloat(0.0f, 1.0f), getRandomUniformFloat(0.0f, 1.0f), getRandomUniformFloat(0.0f, 1.0f) };
        for (unsigned int j = 0; j < 4; j++) {
            colors[i * unit_len + j * 3 + 0] = color.x;
            colors[i * unit_len + j * 3 + 1] = color.y;
            colors[i * unit_len + j * 3 + 2] = color.z;
        }
    }
}


using ProfilerDataType = unsigned long long;
struct Profiler {
    std::vector<std::vector<ProfilerDataType>> data;
    unsigned int current_mark = 0;
    unsigned int current_iteration = 0;
    unsigned int actual_markers_count = 0;
    ProfilerDataType start;
    std::vector<std::string> mark_names;

    static const unsigned int max_marks = 20;
    static const unsigned int history_size = 500'000;

    Profiler() noexcept {
        data.reserve(max_marks);
        mark_names.reserve(max_marks);
        for (unsigned int m = 0; m < max_marks; m++) {
            std::vector<ProfilerDataType> v;
            v.reserve(history_size);
            data.push_back(std::move(v));
        }
    }


    ProfilerDataType get_time() const noexcept {
        timespec t;
        clock_gettime(CLOCK_MONOTONIC_RAW, &t);
        return static_cast<ProfilerDataType>(t.tv_sec) * 1'000'000'000ull + static_cast<ProfilerDataType>(t.tv_nsec);
    }
    ProfilerDataType mark(std::string_view name) noexcept {
        if (current_iteration == 0) {
            mark_names.push_back(std::string(name));
        } else {
            if (mark_names[current_mark] != name) {
                std::cout << "::> Profiler: Markers do not match!!" << '\n';
                std::cout << mark_names[current_mark] << " and " << name << '\n';
                return 0;
            }
        }
        if (current_mark >= max_marks) {
            std::cout << "Max marks count for Profiler reached: " << max_marks << '\n';
            return 0;
        }
        auto& v = data[current_mark++];
        while (v.size() < current_iteration) v.push_back(0);
        const auto time = get_time();
        const auto delta = time - start;
        start = time;
        v.push_back(delta);
        return delta;
    }
    void start_batch() noexcept {
        start = get_time();
    }
    void finish_batch() noexcept {
        current_iteration++;
        actual_markers_count = current_mark;
        current_mark = 0;
    }

    void dump() const noexcept {
        for (unsigned int m = 0; m < actual_markers_count - 1; m++) {
            std::cout << "From [" << mark_names[m] << "] to [" << mark_names[m + 1] << "]:" << '\n';
            unsigned int i = 0;
            for (const ProfilerDataType delta : data[m]) {
                std::cout << "[" << std::setw(7) << i << "]: " << std::setw(9) << delta << " ns" << '\n';
                i++;
            }
        }
    }
};


int main() {
    std::cout << "========================== BEGIN =========================\n";
    GLFWwindow* window = init(global_state.width, global_state.height);
    if (!window) return -1;

    Shader shader_tile("tile.vert", "tile.frag");
    Shader shader_basic("basic_model.vert", "basic_model.frag");

    const auto tile_size = 4; // pixels
    const auto border = 1; // pixels
    FieldSize field_size{ tile_size, border, global_state.width, global_state.height };
    std::vector<float> colors(3 * 4 * field_size.width * field_size.height);
    regenerateColors(colors);
    FieldData field_data = constructField(field_size, colors);
    Matrix4f mvp = Matrix4f::orthographic(0.0f, 1.0f * global_state.width, 0.0f, 1.0f * global_state.height, -1.0f, 20.0f);
    std::cout << "Field size is " << field_size.width << " by " << field_size.height << '\n';

    Vector2f pointer{ 500.0f, 500.0f };
    shader_tile.bind();
    shader_tile.setUniformMat4f("u_MVP", mvp);
    shader_basic.bind();
    shader_basic.setUniformMat4f("u_MVP", mvp);
    shader_basic.setUniformMat4f("u_Model", Matrix4f::identity().translate(50.0f, 800.0f, 0.5f).scale(2300.0f, 200.0f, 1.0f));

    // Graph<float> graph{15*60};
    Graph<float> graph{ 2 * 1024 };
    const float update_fps_every = 0.009f;
    float last_fps_update = 0.0f;
    Profiler profiler;

    Graph<float> line_1{ 2*1024 };
    Graph<float> line_2{ 2*1024 };
    Graph<float> line_3{ 2*1024 };
    Graph<float> line_4{ 2*1024 };
    Graph<float> line_5{ 2*1024 };

    const float change_every = 0.3f;
    float last_change = 0.0f;
    float last_time = 0.0f;
    unsigned int elapsed_times_i = 0;
    std::array<float, 1> elapsed_times;
    profiler.start_batch();
    while (!glfwWindowShouldClose(window)) {
        if (global_state.pause) {
            glfwPollEvents();
            processInput(window, 0.1f);
            continue;
        }
        profiler.mark("loop start");
        if (global_state.resize_recalculation_required) {
            global_state.resize_recalculation_required = false;

            std::cout << "Reconstruction data...\n";
            field_size = FieldSize{ tile_size, border, global_state.width, global_state.height };
            colors = std::vector<float>(3 * 4 * field_size.width * field_size.height);
            regenerateColors(colors);
            field_data.destroy(); // clear all previous buffers
            field_data = constructField(field_size, colors);
            mvp = Matrix4f::orthographic(0.0f, 1.0f * global_state.width, 0.0f, 1.0f * global_state.height, -1.0f, 20.0f);
            std::cout << "New field size is " << field_size.width << " by " << field_size.height << '\n';

            shader_tile.bind();
            shader_tile.setUniformMat4f("u_MVP", mvp);
            shader_basic.bind();
            shader_basic.setUniformMat4f("u_MVP", mvp);
        }


        // Render
        line_1.pushValue(profiler.mark("render start"));
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        profiler.mark("clear done");

        shader_tile.bind();
        glBindVertexArray(field_data.vao);
        glDrawElements(GL_TRIANGLES, field_data.indices_count, GL_UNSIGNED_INT, 0);
        line_2.pushValue(profiler.mark("background draw done"));

        shader_basic.bind();
        shader_basic.setUniformMat4f("u_Model", Matrix4f::identity().translate(50.0f, 1200.0f, 0.5f).scale(2300.0f, 190.0f, 1.0f));
        graph.draw();
        shader_basic.setUniformMat4f("u_Model", Matrix4f::identity().translate(50.0f, 1000.0f, 0.5f).scale(2300.0f, 190.0f, 1.0f));
        line_1.draw();
        shader_basic.setUniformMat4f("u_Model", Matrix4f::identity().translate(50.0f, 800.0f, 0.5f).scale(2300.0f, 190.0f, 1.0f));
        line_2.draw();
        shader_basic.setUniformMat4f("u_Model", Matrix4f::identity().translate(50.0f, 600.0f, 0.5f).scale(2300.0f, 190.0f, 1.0f));
        line_3.draw();
        shader_basic.setUniformMat4f("u_Model", Matrix4f::identity().translate(50.0f, 400.0f, 0.5f).scale(2300.0f, 190.0f, 1.0f));
        line_4.draw();
        shader_basic.setUniformMat4f("u_Model", Matrix4f::identity().translate(50.0f, 200.0f, 0.5f).scale(2300.0f, 190.0f, 1.0f));
        line_5.draw();
        profiler.mark("graph draw done");


        // Update
        const float current_time = glfwGetTime();
        const float delta_time = current_time - last_time;
        last_time = current_time;
        profiler.mark("before glFinish");
        glFinish();
        glfwSwapBuffers(window);
        line_3.pushValue(profiler.mark("buffer swap done"));
        glfwPollEvents();
        processInput(window, delta_time);
        line_4.pushValue(profiler.mark("input processing done"));

        elapsed_times[elapsed_times_i++] = delta_time;
        if (elapsed_times_i >= elapsed_times.size()) elapsed_times_i = 0;
        const float avg_elapsed = std::accumulate(elapsed_times.cbegin(), elapsed_times.cend(), 0.0f) / elapsed_times.size();
        const auto [min_elapsed, max_elapsed] = std::minmax_element(elapsed_times.cbegin(), elapsed_times.cend());
        const float avg_fps = 1.0f / avg_elapsed;
        const float min_fps = 1.0f / (*max_elapsed);
        const float max_fps = 1.0f / (*min_elapsed);
        // std::cout << "Avg elapsed = " << avg_elapsed * 1000.0f << "[" << *min_elapsed * 1000.0f << ";" << *max_elapsed * 1000.0f << "] ms"
        //     << "  Avg FPS = " << avg_fps << "[" << min_fps << ";" << max_fps << "]" << '\n';

        profiler.mark("regenerate start");
        if (last_time - change_every > last_change) {
            regenerateColors(colors);
            glBindVertexArray(0);
            glBindBuffer(GL_ARRAY_BUFFER, field_data.vbo_colors);
            glBufferSubData(GL_ARRAY_BUFFER, 0, colors.size() * sizeof(colors[0]), colors.data());
            last_change = last_time;
        }
        profiler.mark("regenerate finish");

        profiler.mark("graph push start");
        // if (last_time - update_fps_every > last_fps_update) {
            graph.pushValue(avg_elapsed);
            // last_fps_update = last_time;
        // }
        profiler.mark("graph push finish");

        pointer = Vector2f(global_state.mouse_x, global_state.mouse_y);
        shader_tile.bind();
        shader_tile.setUniform2f("u_Pointer", pointer.x, pointer.y);

        line_5.pushValue(profiler.mark("loop finish"));
        profiler.finish_batch();
        profiler.start_batch();
    }

    profiler.dump();

    glfwTerminate();
    std::cout << "=========================== END ==========================\n";
    return 0;
}


void processInput(GLFWwindow *window, float dt) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    } else if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS) {
        global_state.pause = true;
    } else if (glfwGetKey(window, GLFW_KEY_P) == GLFW_RELEASE) {
        global_state.pause = false;
    }
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
    std::cout << ":> Framebuffer callback triggered with width=" << width << " and height=" << height << '\n';

    global_state.resize_recalculation_required = true;
    global_state.width = width;
    global_state.height = height;
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    if (xpos < 0) xpos = 0;
    if (xpos >= global_state.width) xpos = global_state.width;
    if (ypos < 0) ypos = 0;
    if (ypos >= global_state.height) ypos = global_state.height;
    global_state.mouse_x = xpos;
    global_state.mouse_y = global_state.height - ypos;
}
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {}


GLFWwindow* init(unsigned int width, unsigned int height) {
    if (!glfwInit()) {
        std::cerr << ":> Failed at glfwInit()\n";
        return nullptr;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE); // For debug callback
    glfwWindowHint(GLFW_SAMPLES, 1); // Multisampling(antialiasing): 1 == off

    GLFWwindow* window = glfwCreateWindow(width, height, "Graph", NULL, NULL);
    if (!window) {
        std::cout << ":> Failed to create GLFWwindow" << '\n';
        glfwTerminate();
        return nullptr;
    }
    glfwSetWindowPos(window, 50, 50); // .., width, height from top-left corner

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSwapInterval(0); // 0 -- unbounded (may have tearing), 1 -- almost vsync

    if (glewInit() != GLEW_OK) {
        std::cout << ":> Failed at glewInit()" << '\n';
        return nullptr;
    }

    // Enable the debug callback
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(openglCallbackFunction, nullptr);
    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, NULL, true);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    return window;
}


void APIENTRY openglCallbackFunction(
        __attribute__((unused)) GLenum source,
        GLenum type,
        GLuint id,
        GLenum severity,
        __attribute__((unused)) GLsizei length,
        const GLchar* message,
        __attribute__((unused)) const void* userParam) {

    if (severity == GL_DEBUG_SEVERITY_NOTIFICATION) return;
    if (id == 131218) return; // cannot be helped

    std::cout << "---------------------opengl-callback-start------------\n";
    std::cout << "=> Message: " << message << '\n';
    std::cout << "=> Type: ";
    switch (type) {
        case GL_DEBUG_TYPE_ERROR:
            std::cout << "ERROR";
            break;
        case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
            std::cout << "DEPRECATED_BEHAVIOR";
            break;
        case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
            std::cout << "UNDEFINED_BEHAVIOR";
            break;
        case GL_DEBUG_TYPE_PORTABILITY:
            std::cout << "PORTABILITY";
            break;
        case GL_DEBUG_TYPE_PERFORMANCE:
            std::cout << "PERFORMANCE";
            break;
        case GL_DEBUG_TYPE_OTHER:
            std::cout << "OTHER";
            break;
    }
    std::cout << '\n';

    std::cout << "=> Id: " << id << '\n';
    std::cout << "=> Severity: ";
    switch (severity) {
        case GL_DEBUG_SEVERITY_LOW:
            std::cout << "LOW";
            break;
        case GL_DEBUG_SEVERITY_MEDIUM:
            std::cout << "MEDIUM";
            break;
        case GL_DEBUG_SEVERITY_HIGH:
            std::cout << "HIGH";
            break;
        case GL_DEBUG_SEVERITY_NOTIFICATION:
            std::cout << "NOTIFICATION";
            break;
    }
    std::cout << '\n';
    std::cout << "---------------------opengl-callback-end--------------\n";
}
