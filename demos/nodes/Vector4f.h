#ifndef VECTOR4F_H
#define VECTOR4F_H

#include <iostream>

#include "Vector3f.h"


struct Vector4f {
    float x, y, z, w;

    inline Vector4f(float x, float y, float z, float w) noexcept : x(x), y(y), z(z), w(w) {}
    inline Vector4f(float a) noexcept : Vector4f(a, a, a, a) {}
    inline Vector4f() noexcept : Vector4f(0.0f) {}

    void operator+=(const Vector3f& other) noexcept {
        x += other.x;
        y += other.y;
        z += other.z;
    }

    // Vector4f& negate() noexcept { x = -x; y = -y; z = -z; return *this; }

    Vector3f negateUnlift() const noexcept { return Vector3f{ -x, -y, -z }; }
};

std::ostream& operator<<(std::ostream& stream, const Vector4f& vector) noexcept {
    stream << '[' << vector.x << ',' << vector.y << ',' << vector.z << ',' << vector.w << ']';
    return stream;
}


#endif
