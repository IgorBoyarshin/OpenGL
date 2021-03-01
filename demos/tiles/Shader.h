#ifndef SHADER_H
#define SHADER_H


#include <GL/glew.h>
// #include <glm/glm.hpp>

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <unordered_map>

#include "Matrix4f.h"


class Shader {
    private:
        unsigned int m_ID;
        std::unordered_map<std::string, int> m_UniformLocationCache;

    public:
        inline Shader(const std::string& filepathVertex, const std::string& filepathFragment) noexcept :
            m_ID(createShader(readFromFile(filepathVertex), readFromFile(filepathFragment))) {}

        inline ~Shader() noexcept { glDeleteProgram(m_ID); }

        inline void bind()   const noexcept { glUseProgram(m_ID); }
        inline void unbind() const noexcept { glUseProgram(0); }

        inline void setUniform1f(const std::string& name, float f0) noexcept {
            glUniform1f(getUniformLocation(name), f0);
        }
        inline void setUniform1i(const std::string& name, int i0) noexcept {
            glUniform1i(getUniformLocation(name), i0);
        }
        inline void setUniform2f(const std::string& name, float f0, float f1) noexcept {
            glUniform2f(getUniformLocation(name), f0, f1);
        }
        inline void setUniform3f(const std::string& name, const Vector3f& v) noexcept {
            glUniform3f(getUniformLocation(name), v.x, v.y, v.z);
        }
        inline void setUniform3f(const std::string& name, float f0, float f1, float f2) noexcept {
            glUniform3f(getUniformLocation(name), f0, f1, f2);
        }
        inline void setUniform4f(const std::string& name, float f0, float f1, float f2, float f3) noexcept {
            glUniform4f(getUniformLocation(name), f0, f1, f2, f3);
        }
        inline void setUniformMat4f(const std::string& name, const Matrix4f& matrix) noexcept {
            glUniformMatrix4fv(getUniformLocation(name), 1, GL_FALSE, matrix.data);
        }

    private:
        inline std::string readFromFile(const std::string& filepath) {
            std::ifstream stream(filepath);
            std::string line;
            std::stringstream ss;
            while (getline(stream, line)) {
                ss << line << '\n';
            }

            return ss.str();
        }

        inline unsigned int compileShader(unsigned int type, const std::string& source) noexcept {
            unsigned int id = glCreateShader(type);
            const char* src = source.c_str();
            glShaderSource(id, 1, &src, nullptr);
            glCompileShader(id);

            int result;
            glGetShaderiv(id, GL_COMPILE_STATUS, &result);
            if (result == GL_FALSE) {
                int length;
                glGetShaderiv(id, GL_INFO_LOG_LENGTH, &length);
                char* message = (char*) alloca(length * sizeof(char));
                glGetShaderInfoLog(id, length, &length, message);
                std::cout << "Failed to compile ";
                switch (type) {
                    case GL_VERTEX_SHADER:
                        std::cout << "vertex shader";
                        break;
                    case GL_FRAGMENT_SHADER:
                        std::cout << "fragment shader";
                        break;
                }
                std::cout << ".\n";
                std::cout << message << '\n';
                glDeleteShader(id);
                return 0;
            }

            return id;
        }

        inline unsigned int createShader(const std::string& vertexShader, const std::string& fragmentShader) noexcept {
            unsigned int program = glCreateProgram();
            unsigned int vs = compileShader(GL_VERTEX_SHADER, vertexShader);
            unsigned int fs = compileShader(GL_FRAGMENT_SHADER, fragmentShader);

            glAttachShader(program, vs);
            glAttachShader(program, fs);
            glLinkProgram(program);
            glValidateProgram(program);

            glDeleteShader(vs);
            glDeleteShader(fs);

            return program;
        }

        // XXX Uniforms in OpenGL are signed int!!!
        inline int getUniformLocation(const std::string& name) noexcept {
            if (m_UniformLocationCache.find(name) != m_UniformLocationCache.end()) {
                return m_UniformLocationCache[name];
            }

            const int location = glGetUniformLocation(m_ID, name.c_str());
            if (location == -1) {
                std::cout << ":> Shader: Uniform " << name << " is unused or invalid.\n";
            } else {
                m_UniformLocationCache[name] = location;
            }

            return location;
        }
};


#endif
