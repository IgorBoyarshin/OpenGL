#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <vector>
#include <array>
#include <random>

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


int main() {
    std::cout << "========================== BEGIN =========================\n";
    GLFWwindow* window = init(global_state.width, global_state.height);
    if (!window) return -1;

    Shader shader("tile.vert", "tile.frag");
    shader.bind();

    const auto tile_size = 15; // pixels
    const auto border = 60; // pixels
    FieldSize field_size{ tile_size, border, global_state.width, global_state.height };
    std::vector<float> colors(3 * 4 * field_size.width * field_size.height);
    regenerateColors(colors);
    FieldData field_data = constructField(field_size, colors);
    Matrix4f mvp = Matrix4f::orthographic(0.0f, 1.0f * global_state.width, 0.0f, 1.0f * global_state.height, -0.1f, 20.0f);
    std::cout << "Field size is " << field_size.width << " by " << field_size.height << '\n';

    Vector2f pointer{ 500.0f, 500.0f };
    shader.setUniformMat4f("u_MVP", mvp);

    const float change_every = 0.2f;
    float last_change = 0.0f;
    float last_time = 0.0f;
    while (!glfwWindowShouldClose(window)) {
        if (global_state.resize_recalculation_required) {
            global_state.resize_recalculation_required = false;

            std::cout << "Reconstruction data...\n";
            field_size = FieldSize{ tile_size, border, global_state.width, global_state.height };
            colors = std::vector<float>(3 * 4 * field_size.width * field_size.height);
            regenerateColors(colors);
            field_data.destroy(); // clear all previous buffers
            field_data = constructField(field_size, colors);
            mvp = Matrix4f::orthographic(0.0f, 1.0f * global_state.width, 0.0f, 1.0f * global_state.height, -0.1f, 20.0f);
            std::cout << "New field size is " << field_size.width << " by " << field_size.height << '\n';

            shader.setUniformMat4f("u_MVP", mvp);
        }


        // Render
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glBindVertexArray(field_data.vao);
        glDrawElements(GL_TRIANGLES, field_data.indices_count, GL_UNSIGNED_INT, 0);


        // Update
        const float current_time = glfwGetTime();
        const float delta_time = current_time - last_time;
        last_time = current_time;
        glfwSwapBuffers(window);
        glfwPollEvents();
        processInput(window, delta_time);

        if (last_time - change_every > last_change) {
            regenerateColors(colors);
            glBindVertexArray(0);
            glBindBuffer(GL_ARRAY_BUFFER, field_data.vbo_colors);
            glBufferSubData(GL_ARRAY_BUFFER, 0, colors.size() * sizeof(colors[0]), colors.data());
            last_change = last_time;
        }

        pointer = Vector2f(global_state.mouse_x, global_state.mouse_y);
        shader.setUniform2f("u_Pointer", pointer.x, pointer.y);
    }

    glfwTerminate();
    std::cout << "=========================== END ==========================\n";
    return 0;
}


void processInput(GLFWwindow *window, float dt) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
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

    GLFWwindow* window = glfwCreateWindow(width, height, "Tiling rainbow", NULL, NULL);
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
    glfwSwapInterval(1);

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
