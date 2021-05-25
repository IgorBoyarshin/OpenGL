#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <vector>
#include <array>
#include <random>

#include "Shader.h"
#include "Texture.h"


// Will get set in resize callback
int WIDTH = 1920;
int HEIGHT = 1080;
Matrix4f mvp = Matrix4f::orthographic(0.0f, WIDTH * 1.0f, 0.0f, HEIGHT * 1.0f, 0.1f, 20.0f)
    * Matrix4f::identity().translate(Vector3f{0.0f, 0.0f, 1.0f});
Matrix4f girl_model;


void APIENTRY openglCallbackFunction(
        __attribute__((unused)) GLenum source,
        GLenum type,
        GLuint id,
        GLenum severity,
        __attribute__((unused)) GLsizei length,
        const GLchar* message,
        __attribute__((unused)) const void* userParam
);

void glfwErrorCallbackFunction(int error, const char* description);

GLFWwindow* init();
void processInput(GLFWwindow *window, float dt);


struct ObjectData {
    unsigned int idVao;
    unsigned int idVbo;
    unsigned int idIbo;
    unsigned int indicesCount;
    inline ObjectData(unsigned int idVao, unsigned int idVbo,
            unsigned int idIbo, unsigned int indicesCount) noexcept
        : idVao(idVao), idVbo(idVbo), idIbo(idIbo), indicesCount(indicesCount) {}
};

ObjectData constructSquare() noexcept;


Matrix4f calculateGirlModel(int window_width, int window_height) noexcept {
    const auto GIRL_WIDTH = 1920;
    const auto GIRL_HEIGHT = 1080;
    const auto girl_aspect_ratio = static_cast<float>(GIRL_WIDTH) / static_cast<float>(GIRL_HEIGHT);
    const auto window_aspect_ratio = static_cast<float>(window_width) / static_cast<float>(window_height);
    std::cout << "window asp=" << window_aspect_ratio << '\n';
    const auto ratio = 0.8f;
    if (window_aspect_ratio > girl_aspect_ratio) {
        const auto height = ratio * window_height;
        const auto width = height * girl_aspect_ratio;
        const auto shift_y = window_height * (1.0f - ratio) / 2.0f;
        const auto shift_x = (window_width - width) / 2.0f;
        return Matrix4f::identity().translate(shift_x, shift_y, 0.0f).scale(width, height, 1.0f);
    } else {
        const auto width = ratio * window_width;
        const auto height = width / girl_aspect_ratio;
        const auto shift_x = window_width * (1.0f - ratio) / 2.0f;
        const auto shift_y = (window_height - height) / 2.0f;
        return Matrix4f::identity().translate(shift_x, shift_y, 0.0f).scale(width, height, 1.0f);
    }
}



int main() {
    std::cout << "========================== BEGIN =========================\n";
    GLFWwindow* window = init();
    if (!window) return -1;

    const unsigned int girl_1_texture_id = 0;
    const unsigned int girl_2_texture_id = 1;
    const unsigned int girl_3_texture_id = 2;
    Texture girl_1_texture("assets/girl_1.png");
    Texture girl_2_texture("assets/girl_2.png");
    Texture girl_3_texture("assets/girl_3.png");
    girl_1_texture.bind(girl_1_texture_id);
    girl_2_texture.bind(girl_2_texture_id);
    girl_3_texture.bind(girl_3_texture_id);

    const auto& [squareVao, squareVbo, squareIbo, squareIndicesCount] = constructSquare();
    girl_model = calculateGirlModel(WIDTH, HEIGHT);


    Shader girl_shader("girl.vert", "girl.frag");



    float lastTime = 0.0f;
    while (!glfwWindowShouldClose(window)) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glBindVertexArray(squareVao);

        // Should only be called on window size update...
        girl_shader.bind();
        girl_shader.setUniformMat4f("u_MVP", mvp);
        girl_shader.setUniformMat4f("u_Model", girl_model);

        const int state = static_cast<int>(lastTime / 2) % 3;
        const float alpha = lastTime / 2 - static_cast<int>(lastTime / 2);
        switch (state) {
            case 0:
                girl_shader.setUniform1i("u_Texture1", girl_1_texture_id);
                girl_shader.setUniform1i("u_Texture2", girl_2_texture_id);
                break;
            case 1:
                girl_shader.setUniform1i("u_Texture1", girl_2_texture_id);
                girl_shader.setUniform1i("u_Texture2", girl_3_texture_id);
                break;
            case 2:
                girl_shader.setUniform1i("u_Texture1", girl_3_texture_id);
                girl_shader.setUniform1i("u_Texture2", girl_1_texture_id);
                break;
        }

        girl_shader.setUniform4f("u_Color", 0.1f, 0.0f, 0.0f, alpha);
        glDrawElements(GL_TRIANGLES, squareIndicesCount, GL_UNSIGNED_INT, 0);

        glBindVertexArray(0);
        girl_shader.unbind();

        const float currentTime = glfwGetTime();
        const float deltaTime = currentTime - lastTime;
        lastTime = currentTime;
        glfwSwapBuffers(window);
        glfwPollEvents();
        processInput(window, deltaTime);
    }

    glfwTerminate();
    std::cout << "=========================== END ==========================\n";
    return 0;
}


ObjectData constructSquare() noexcept {
    const float data[] = {
        // Vertices
        0.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,

        // Texture coordinates
        0.0f, 0.0f,
        1.0f, 0.0f,
        0.0f, 1.0f,
        1.0f, 1.0f
    };

    const unsigned int indices[] = { // CW
        0, 2, 1, 1, 2, 3
    };


    unsigned int idVao;
    glGenVertexArrays(1, &idVao);
    glBindVertexArray(idVao);
        // VBO
        unsigned int idVbo;
        glGenBuffers(1, &idVbo);
        glBindBuffer(GL_ARRAY_BUFFER, idVbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(data), data, GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
        const auto POSITION_COUNT = 3;
        const auto POSITION_STRIDE = POSITION_COUNT * sizeof(float);
        const auto POSITION_OFFSET = 0;
        glVertexAttribPointer(0, POSITION_COUNT, GL_FLOAT, GL_FALSE,
                POSITION_STRIDE, reinterpret_cast<const void*>(POSITION_OFFSET));
        const auto TEXTURE_COUNT = 2;
        const auto TEXTURE_STRIDE = TEXTURE_COUNT * sizeof(float);
        const auto TEXTURE_OFFSET = 12 * sizeof(float);
        glVertexAttribPointer(1, TEXTURE_COUNT, GL_FLOAT, GL_FALSE,
                TEXTURE_STRIDE, reinterpret_cast<const void*>(TEXTURE_OFFSET));

        // IBO
        unsigned int idIbo;
        glGenBuffers(1, &idIbo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, idIbo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    return ObjectData{ idVao, idVbo, idIbo, 6 };
}


void processInput(GLFWwindow *window, float dt) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
    std::cout << "Framebuffer callback triggered with width=" << width << " and height=" << height << '\n';

    WIDTH = width;
    HEIGHT = height;
    mvp = Matrix4f::orthographic(0.0f, WIDTH * 1.0f, 0.0f, HEIGHT * 1.0f, 0.1f, 20.0f)
        * Matrix4f::identity().translate(Vector3f{0.0f, 0.0f, 1.0f});
    girl_model = calculateGirlModel(WIDTH, HEIGHT);
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos) {}
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {}


GLFWwindow* init() {
    glfwSetErrorCallback(glfwErrorCallbackFunction);

    if (!glfwInit()) {
        std::cerr << ":> Failed at glfwInit()\n";
        return nullptr;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE); // For debug callback
    glfwWindowHint(GLFW_SAMPLES, 1); // 1 == off

    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "OpenGL Test", NULL, NULL);
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


void glfwErrorCallbackFunction(int error, const char* description) {
    std::cout << "A GLFW error [" << error << "] occured:\n";
    std::cout << description << '\n';
    std::cout << std::endl;
}
