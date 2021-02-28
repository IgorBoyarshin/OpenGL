#ifndef TEXTURE_H
#define TEXTURE_H

#include <GL/glew.h>

#include <string>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"


class Texture {
    private:
        unsigned int m_ID;
        int m_Width, m_Height;

    public:
        inline Texture(const std::string& filepath) :
            m_ID(0), m_Width(0), m_Height(0) {
                stbi_set_flip_vertically_on_load(true);
                int componentsCount;
                unsigned char* buffer = stbi_load(filepath.c_str(), &m_Width, &m_Height, &componentsCount, 0);
                GLenum format;
                if      (componentsCount == 1) format = GL_RED;
                else if (componentsCount == 3) format = GL_RGB;
                else if (componentsCount == 4) format = GL_RGBA;
                else assert(false);

                glGenTextures(1, &m_ID);
                glBindTexture(GL_TEXTURE_2D, m_ID);

                // GL_NEAREST, GL_LINEAR (smooth)
                // GL_NEAREST_MIPMAP_LINEAR: linearly interpolates between the
                // two mipmaps that most closely match the size of a pixel and
                // samples the interpolated level via nearest neighbor interpolation.
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); // no MIPMAPs here!
                // GL_REPEAT, GL_MIRRORED_REPEAT, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_BORDER
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                glTexImage2D(GL_TEXTURE_2D, 0, format, m_Width, m_Height, 0, format, GL_UNSIGNED_BYTE, buffer);
                // TODO: generate mipmaps
                glGenerateMipmap(GL_TEXTURE_2D);

                glBindTexture(GL_TEXTURE_2D, 0);

                // Could also store this data
                if (buffer) stbi_image_free(buffer);
            }

        inline ~Texture() {
            glDeleteTextures(1, &m_ID);
        }

        inline void bind(unsigned int slot /*= 0*/) const {
            glActiveTexture(GL_TEXTURE0 + slot);
            glBindTexture(GL_TEXTURE_2D, m_ID);
        }

        inline void unbind() const {
            glBindTexture(GL_TEXTURE_2D, 0);
        }

        inline int getWidth()  const { return m_Width; }
        inline int getHeight() const { return m_Height; }
};


#endif
