#ifndef VECTOR3F_H
#define VECTOR3F_H

#include <cmath>
#include <iostream>


struct Vector3f {
    float x, y, z;

    inline Vector3f(float x, float y, float z) noexcept : x(x), y(y), z(z) {}
    inline Vector3f(float a) noexcept : Vector3f(a, a, a) {}
    inline Vector3f() noexcept : Vector3f(0.0f) {}

    Vector3f operator*(float scalar) const noexcept {
        return Vector3f{ x * scalar, y * scalar, z * scalar };
    }

    Vector3f operator*(const Vector3f& other) const noexcept {
        return Vector3f{ x * other.x, y * other.y, z * other.z };
    }

    Vector3f operator+(const Vector3f& other) const noexcept {
        return Vector3f{ x + other.x, y + other.y, z + other.z };
    }

    Vector3f operator-(const Vector3f& other) const noexcept {
        return Vector3f{ x - other.x, y - other.y, z - other.z };
    }

    void operator+=(const Vector3f& other) noexcept {
        x += other.x;
        y += other.y;
        z += other.z;
    }

    Vector3f cross(const Vector3f& other) const noexcept {
        return Vector3f{
            y * other.z - z * other.y,
            z * other.x - x * other.z,
            x * other.y - y * other.x
        };
    }

    Vector3f& normalize() noexcept {
        const float lengthSqrt = std::sqrt(x * x + y * y + z * z);
        x /= lengthSqrt;
        y /= lengthSqrt;
        z /= lengthSqrt;
        return *this;
    }

    Vector3f negated() noexcept {
        return Vector3f{ -x, -y, -z };
    }
};


std::ostream& operator<<(std::ostream& stream, const Vector3f& vector) noexcept {
    stream << '[' << vector.x << ',' << vector.y << ',' << vector.z << ']';
    return stream;
}


#endif
