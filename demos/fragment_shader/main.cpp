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


struct GlobalState {
    bool resize_recalculation_required = false;
    unsigned int width = 1024;
    unsigned int height = 1024;
    unsigned int mouse_x = 0;
    unsigned int mouse_y = 0;
    bool pause = false;
};
static GlobalState global_state;


GLFWwindow* init(unsigned int widht, unsigned int height);
void processInput(GLFWwindow *window, float dt);

void glfwErrorCallbackFunction(int error, const char* description);

int main() {
    std::cout << "========================== BEGIN =========================\n";
    GLFWwindow* window = init(global_state.width, global_state.height);
    if (!window) return -1;

    unsigned int vao;
    unsigned int vbo;
    unsigned int ibo;
    unsigned int indices_count;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    {
        const unsigned int size = 2 * 4;
        const float data[size] = {
            // x, y
            -1.0f, -1.0f,
            -1.0f,  1.0f,
             1.0f,  1.0f,
             1.0f, -1.0f
        };
        const unsigned int size_bytes = size * sizeof(data[0]);
        glGenBuffers(1, &vbo);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, size_bytes, data, GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        const auto floats_per_vertex = 2; // x, y
        const auto stride_bytes = floats_per_vertex * sizeof(float);
        const auto offset = 0;
        glVertexAttribPointer(0, floats_per_vertex, GL_FLOAT, GL_FALSE, stride_bytes, reinterpret_cast<const void*>(offset));
    }
    {
        const unsigned int size = 6;
        const unsigned int data[size] = {
            0, 1, 2, 0, 2, 3
        };
        indices_count = 6;
        const unsigned int size_bytes = size * sizeof(data[0]);
        glGenBuffers(1, &ibo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, size_bytes, data, GL_STATIC_DRAW);
    }

    Shader shader("basic.vert", "field.frag");
    shader.bind();
    shader.setUniform2f("u_resolution", 1.0f * global_state.width, 1.0f * global_state.height);

    float last_time = 0.0f;
    while (!glfwWindowShouldClose(window)) {
        if (global_state.pause) {
            glfwPollEvents();
            processInput(window, 0.0f);
            continue;
        }
        if (global_state.resize_recalculation_required) {
            global_state.resize_recalculation_required = false;
            shader.setUniform2f("u_resolution", 1.0f * global_state.width, 1.0f * global_state.height);
        }

        // Render
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glDrawElements(GL_TRIANGLES, indices_count, GL_UNSIGNED_INT, 0);
        glfwSwapBuffers(window);

        // Update
        const float current_time = glfwGetTime();
        const float delta_time = current_time - last_time;
        last_time = current_time;

        glfwPollEvents();
        processInput(window, delta_time);

        shader.setUniform1f("u_time", current_time);

        // std::cout << "Delta time = " << delta_time * 1000.0 << "ms\n";
    }

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


GLFWwindow* init(unsigned int width, unsigned int height) {
    glfwSetErrorCallback(glfwErrorCallbackFunction);

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
    // glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSwapInterval(1); // 0 -- unbounded (may have tearing), 1 -- almost vsync

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


void glfwErrorCallbackFunction(int error, const char* description) {
    std::cout << "A GLFW error [" << error << "] occured:\n";
    std::cout << description << '\n';
    std::cout << std::endl;
}
