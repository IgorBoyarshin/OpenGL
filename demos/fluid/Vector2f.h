#ifndef VECTOR2F_H
#define VECTOR2F_H

#include <iostream>
#include <cmath>


struct Vector2f {
    float x, y;

    inline Vector2f(float x, float y) noexcept : x(x), y(y) {}
    inline Vector2f() noexcept : Vector2f(0.0f, 0.0f) {}

    Vector2f operator+(const Vector2f& other) const noexcept {
        return Vector2f{ x + other.x, y + other.y };
    }

    Vector2f operator-(const Vector2f& other) const noexcept {
        return Vector2f{ x - other.x, y - other.y };
    }

    void operator+=(const Vector2f& other) noexcept {
        x += other.x;
        y += other.y;
    }

    void operator*=(const float f) noexcept {
        x *= f;
        y *= f;
    }

    Vector2f operator*(const float f) const noexcept {
        return Vector2f{ x * f, y * f};
    }

    Vector2f operator/(const float f) const noexcept {
        return Vector2f{ x / f, y / f};
    }

    Vector2f negated() noexcept {
        return Vector2f{ -x, -y };
    }

    void normalize() noexcept {
        const float l = length();
        x /= l;
        y /= l;
    }

    Vector2f& normalized() noexcept {
        const float l = length();
        x /= l;
        y /= l;
        return *this;
    }

    bool not_zero() noexcept {
        static constexpr float e = 0.0000001f;
        return (std::abs(x) > e) && (std::abs(y) > e);
    }

    float length() const noexcept {
        return std::sqrt(x*x + y*y);
    }
    float length_sqr() const noexcept {
        return x*x + y*y;
    }
};


std::ostream& operator<<(std::ostream& stream, const Vector2f& v) noexcept {
    stream << "[" << v.x << ";" << v.y << "]";
    return stream;
}


#endif
