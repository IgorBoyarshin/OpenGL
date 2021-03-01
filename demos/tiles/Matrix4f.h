#ifndef MATRIX_H
#define MATRIX_H

#include <cstring>
#include <iostream>
#include <cmath>

#include "Vector4f.h"
#include "Vector3f.h"

static constexpr auto PI = 3.141592653589793238462643383279;

struct Matrix4f {
    // Column-major order
    float data[16];

    inline Matrix4f() noexcept {}
    inline Matrix4f(const float from[16]) noexcept {
        memcpy(data, from, 16 * sizeof(float));
    }

    // inline Matrix4f(float diagonal) noexcept : Matrix4f() {
    // 	memset(data, 0, 16 * sizeof(float));
    //     for (int i = 0; i < 4; i++) data[4 * i + i] = diagonal;
    // }

    inline void zero() noexcept {
        memset(data, 0, 16 * sizeof(float));
    }

    static inline Matrix4f identity() noexcept {
        Matrix4f matrix = zeroed();
        for (int i = 0; i < 4; i++) matrix[4 * i + i] = 1.0f;
        return matrix;
    }

    static inline Matrix4f zeroed() noexcept {
        Matrix4f matrix;
        matrix.zero();
        return matrix;
    }

    inline float operator[](unsigned int i) const noexcept {
#ifndef NDEBUG
        if (i >= 16) {
            std::cerr << ":> Matrix4f: access of field " << i
                << ", but Matrix has only " << 16 << " fields.\n";
            return 0;
        }
#endif
        return data[i];
    }

    inline float& operator[](unsigned int i) noexcept {
#ifndef NDEBUG
        if (i >= 16) {
            std::cerr << ":> Matrix4f: access of field " << i
                << ", but Matrix has only " << 16 << " fields.\n";
            return data[0];
        }
#endif
        return data[i];
    }


    static inline Matrix4f orthographic(
            float left, float right, float bottom, float top, float near, float far) noexcept {
        Matrix4f result = Matrix4f::zeroed();
        result[0 + 0 * 4] = 2.0f / (right - left);
        result[1 + 1 * 4] = 2.0f / (top - bottom);
        result[2 + 2 * 4] = 2.0f / (near - far);
        result[0 + 3 * 4] = (left + right) / (left - right);
        result[1 + 3 * 4] = (bottom + top) / (bottom - top);
        result[2 + 3 * 4] = (far + near) / (far - near);
        result[15] = 1.0f;

        return result;
    }

    static inline Matrix4f perspective(
            float width, float height, float fFovDeg, float fzNear, float fzFar) noexcept {
        const float fFovRadHalf = fFovDeg * PI / 360.0f;
        const float fFrustumScale = 1.0f / std::tan(fFovRadHalf);

        Matrix4f result = Matrix4f::zeroed();
        result[0] = fFrustumScale / (width / height);
        result[5] = fFrustumScale;
        result[10] = (fzFar + fzNear) / (fzNear - fzFar);
        result[14] = (2.0f * fzFar * fzNear) / (fzNear - fzFar);
        result[11] = -1.0f;

        return result;
    }

    // Does not alter this matrix
    inline Vector4f operator*(const Vector4f& other) const noexcept {
        Vector4f result;
        result.x += data[0 * 4 + 0] * other.x;
        result.y += data[0 * 4 + 1] * other.x;
        result.z += data[0 * 4 + 2] * other.x;
        result.w += data[0 * 4 + 3] * other.x;
        result.x += data[1 * 4 + 0] * other.y;
        result.y += data[1 * 4 + 1] * other.y;
        result.z += data[1 * 4 + 2] * other.y;
        result.w += data[1 * 4 + 3] * other.y;
        result.x += data[2 * 4 + 0] * other.z;
        result.y += data[2 * 4 + 1] * other.z;
        result.z += data[2 * 4 + 2] * other.z;
        result.w += data[2 * 4 + 3] * other.z;
        result.x += data[3 * 4 + 0] * other.w;
        result.y += data[3 * 4 + 1] * other.w;
        result.z += data[3 * 4 + 2] * other.w;
        result.w += data[3 * 4 + 3] * other.w;

        return result;
    }

    // Does not alter this matrix
    inline Matrix4f operator*(const Matrix4f& other) const noexcept {
        Matrix4f result;
        for (int row = 0; row < 4; row++) {
            for (int column = 0; column < 4; column++) {
                float element = 0.0f;
                for (int index = 0; index < 4; index++) {
                    element += data[4 * index + row] * other[4 * column + index];
                }
                result[4 * column + row] = element;
            }
        }

        return result;
    }

    // Does not alter this matrix
    inline Matrix4f scaled(const Vector3f& vector) const {
        Matrix4f result;
        for (int i = 0; i < 4; i++) result[4 * 0 + i] = data[4 * 0 + i] * vector.x;
        for (int i = 0; i < 4; i++) result[4 * 0 + i] = data[4 * 1 + i] * vector.y;
        for (int i = 0; i < 4; i++) result[4 * 0 + i] = data[4 * 2 + i] * vector.z;
        for (int i = 0; i < 4; i++) result[4 * 0 + i] = data[4 * 3 + i];
        return result;
    }

    // Alters this matrix
    inline Matrix4f& scale(const Vector3f& vector) {
        return scale(vector.x, vector.y, vector.z);
    }

    // Alters this matrix
    inline Matrix4f& scale(float x, float y, float z) {
        for (int i = 0; i < 4; i++) data[4 * 0 + i] *= x;
        for (int i = 0; i < 4; i++) data[4 * 1 + i] *= y;
        for (int i = 0; i < 4; i++) data[4 * 2 + i] *= z;
        return *this;
    }

    // Does not alter this matrix.
    inline Matrix4f translated(const Vector3f& vector) const {
        Matrix4f result;
        memcpy(result.data, data, 4 * 3 * sizeof(float)); // first 3 columns
        for (int i = 0; i < 4; i++) {
            float value = 0.0f;
            value += data[0 * 4 + i] * vector.x;
            value += data[1 * 4 + i] * vector.y;
            value += data[2 * 4 + i] * vector.z;
            value += data[3 * 4 + i];
            result[4 * 3 + i] = value;
        }
        return result;
    }

    // Alters this matrix
    inline Matrix4f& translate(const Vector3f& vector) {
        return translate(vector.x, vector.y, vector.z);
    }

    // Alters this matrix
    inline Matrix4f& translate(float x, float y, float z) {
        for (int i = 0; i < 4; i++) {
            float value = 0.0f;
            value += data[0 * 4 + i] * x;
            value += data[1 * 4 + i] * y;
            value += data[2 * 4 + i] * z;
            value += data[3 * 4 + i];
            data[4 * 3 + i] = value;
        }
        return *this;
    }

    // Does not alter this matrix.
    inline Matrix4f& rotate(float angle, float x, float y, float z) {
        Matrix4f result = (*this) * rotation(angle, x, y, z);
        memcpy(data, result.data, 16 * sizeof(float));
        return *this;
        // return this.multiply(rotation(angle, x, y, z));
    }

    // Angle in degrees
    static inline Matrix4f rotation(float angle, float x, float y, float z) {
        Matrix4f result = identity();
        const float r = angle * PI / 180.0f;;
        const float cos = std::cos(r);
        const float sin = std::sin(r);
        const float omc = 1.0f - cos;

        result[0 + 0 * 4] = x * x * omc + cos; // XXX double arg
        result[1 + 0 * 4] = y * x * omc + z * sin;
        result[2 + 0 * 4] = x * z * omc - y * sin;

        result[0 + 1 * 4] = x * y * omc - z * sin;
        result[1 + 1 * 4] = y * y * omc + cos; // XXX double arg
        result[2 + 1 * 4] = y * z * omc + x * sin;

        result[0 + 2 * 4] = x * z * omc + y * sin;
        result[1 + 2 * 4] = y * z * omc - x * sin;
        result[2 + 2 * 4] = z * z * omc + cos; // XXX double arg

        return result;
    }

    // Does not alter this matrix.
    inline Matrix4f rotateAboutAxis(float angle, Vector3f axis) {
        Matrix4f result = (*this) * rotationAboutAxis(angle, axis);
        memcpy(data, result.data, 16 * sizeof(float));
        return *this;
        // return this.multiply(rotationAboutAxis(angle, axis));
    }

    // Angle in degrees
    static inline Matrix4f rotationAboutAxis(float angle, Vector3f axis) {
        Matrix4f result = identity();
        const float argument = angle * PI / 360.0f;
        const float sin = std::sin(argument);
        const float q0 = std::cos(argument);
        const float q1 = sin * axis.x;
        const float q2 = sin * axis.y;
        const float q3 = sin * axis.z;
        const float q02 = q0 * q0;
        const float q12 = q1 * q1;
        const float q22 = q2 * q2;
        const float q32 = q3 * q3;

        result[0 + 0 * 4] = q02 + q12 - q22 - q32;
        result[1 + 0 * 4] = 2.0f * (q2 * q1 + q0 * q3);
        result[2 + 0 * 4] = 2.0f * (q3 * q1 - q0 * q2);

        result[0 + 1 * 4] = 2.0f * (q1 * q2 - q0 * q3);
        result[1 + 1 * 4] = q02 - q12 + q22 - q32;
        result[2 + 1 * 4] = 2.0f * (q3 * q2 + q0 * q1);

        result[0 + 2 * 4] = 2.0f * (q1 * q3 + q0 * q2);
        result[1 + 2 * 4] = 2.0f * (q2 * q3 - q0 * q1);
        result[2 + 2 * 4] = q02 - q12 - q22 + q32;

        return result;
    }
};


#endif
