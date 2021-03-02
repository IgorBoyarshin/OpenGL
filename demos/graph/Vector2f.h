#ifndef VECTOR2F_H
#define VECTOR2F_H

#include <iostream>


struct Vector2f {
    float x, y;

    inline Vector2f(float x, float y) noexcept : x(x), y(y) {}
    inline Vector2f() noexcept : Vector2f(0.0f, 0.0f) {}

    Vector2f operator+(const Vector2f& other) const noexcept {
        return Vector2f{ x + other.x, y + other.y };
    }
};


#endif
