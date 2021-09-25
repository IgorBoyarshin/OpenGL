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

#include "Game.h"


struct GlobalState {
    bool resize_recalculation_required = false;
    unsigned int width = 1024;
    unsigned int height = 1024;
    bool mouse_updated = false;
    float mouse_x = 0.0f;
    float mouse_y = 0.0f;
    bool pause = false;
};
static GlobalState global_state;


GLFWwindow* init(unsigned int widht, unsigned int height);

float get_time_micros() noexcept {
    timespec t;
    clock_gettime(CLOCK_MONOTONIC_RAW, &t);
    return static_cast<float>(t.tv_sec) * 1'000'000.0f + static_cast<float>(t.tv_nsec) / 1000.0f;
}

int main() {
    std::cout << "========================== BEGIN =========================\n";
    GLFWwindow* window = init(global_state.width, global_state.height);
    if (!window) return -1;

    Game game(global_state.width, global_state.height);

    float last_time = get_time_micros();
    while (!glfwWindowShouldClose(window)) {
        const float current_time = get_time_micros();
        const float delta_time = current_time - last_time;
        last_time = current_time;
        // std::cout << "Loop delta time = " << std::setw(5) << delta_time << " micros\n";

        glfwPollEvents();
        // if (global_state.mouse_updated) {
        //     global_state.mouse_updated = false;
        //     game.register_mouse(global_state.mouse_x, global_state.mouse_y);
        // }
        game.register_input(window);
        if (game.should_close()) glfwSetWindowShouldClose(window, true);

        if (global_state.pause) continue;

        if (global_state.resize_recalculation_required) {
            global_state.resize_recalculation_required = false;
            game.reset_dimensions(global_state.width, global_state.height);
        }

        // Render
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        game.update_and_render(delta_time);
        glfwSwapBuffers(window);


        // const float frame_delta = get_time_micros() - current_time;
        // std::cout << "Loop delta time = " << std::setw(5) << delta_time << " micros; Frame delta time = " << std::setw(5) << frame_delta << " micros\n";
    }

    glfwTerminate();
    std::cout << "=========================== END ==========================\n";
    return 0;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
    std::cout << ":> Framebuffer callback triggered with width=" << width << " and height=" << height << '\n';

    global_state.resize_recalculation_required = true;
    global_state.width = width;
    global_state.height = height;
}

// void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
//     // if (xpos < 0) xpos = 0;
//     // if (xpos >= global_state.width) xpos = global_state.width;
//     // if (ypos < 0) ypos = 0;
//     // if (ypos >= global_state.height) ypos = global_state.height;
//     global_state.mouse_updated = true;
//     global_state.mouse_x = xpos;
//     global_state.mouse_y = ypos;
//     // global_state.mouse_y = global_state.height - ypos;
// }

// void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {}

// void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {}




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
    if (!glfwInit()) {
        std::cerr << ":> Failed at glfwInit()\n";
        return nullptr;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE); // For debug callback
    glfwWindowHint(GLFW_SAMPLES, 1); // Multisampling(antialiasing): 1 == off

    GLFWwindow* window = glfwCreateWindow(width, height, "Flow Game", NULL, NULL);
    if (!window) {
        std::cout << ":> Failed to create GLFWwindow" << '\n';
        glfwTerminate();
        return nullptr;
    }
    glfwSetWindowPos(window, 50, 50); // .., width, height from top-left corner

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    // glfwSetMouseButtonCallback(window, mouse_button_callback);
    // glfwSetCursorPosCallback(window, mouse_callback);
    // glfwSetScrollCallback(window, scroll_callback);
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

    std::cout << "OpenGL Vendor:   " << glGetString(GL_VENDOR) << '\n';
    std::cout << "OpenGL Renderer: " << glGetString(GL_RENDERER) << '\n';

    std::cout << "========================== START =========================\n";

    return window;
}
