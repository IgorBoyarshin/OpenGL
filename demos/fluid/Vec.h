#ifndef VEC_H
#define VEC_H

#include <cmath>
#include <iostream>

// template<typename T>
// concept Float = std::floating_point<T>;

template<unsigned int DIM>
struct Vec {
    float d[DIM];

    template<typename ...T>
    Vec(T&& ...args) noexcept : d(args...) {}

    Vec(float def) noexcept {
        for (unsigned int i = 0; i < DIM; i++) d[i] = def;
    }

    Vec() noexcept {}

    float  operator[](unsigned int i) const noexcept { return d[i]; }
    float& operator[](unsigned int i)       noexcept { return d[i]; }

    Vec<DIM> operator*(float scalar) const noexcept {
        Vec<DIM> res;
        for (unsigned int i = 0; i < DIM; i++) res[i] = d[i] * scalar;
        return res;
    }

    Vec<DIM> operator/(float scalar) const noexcept {
        Vec<DIM> res;
        for (unsigned int i = 0; i < DIM; i++) res[i] = d[i] / scalar;
        return res;
    }

    // Vec<DIM>& operator/=(const Vec<DIM>& other) const noexcept {
    //     for (unsigned int i = 0; i < DIM; i++) d[i] /= other[i];
    //     return *this;
    // }

    Vec<DIM>& operator*=(float scalar) noexcept {
        for (unsigned int i = 0; i < DIM; i++) d[i] *= scalar;
        return *this;
    }

    Vec<DIM> operator*(const Vec<DIM>& other) const noexcept {
        Vec<DIM> res;
        for (unsigned int i = 0; i < DIM; i++) res[i] = d[i] * other[i];
        return res;
    }

    Vec<DIM> operator+(const Vec<DIM>& other) const noexcept {
        Vec<DIM> res;
        for (unsigned int i = 0; i < DIM; i++) res[i] = d[i] + other[i];
        return res;
    }

    Vec<DIM> operator-(const Vec<DIM>& other) const noexcept {
        Vec<DIM> res;
        for (unsigned int i = 0; i < DIM; i++) res[i] = d[i] - other[i];
        return res;
    }

    void operator+=(const Vec<DIM>& other) noexcept {
        for (unsigned int i = 0; i < DIM; i++) d[i] += other[i];
    }

    Vec<DIM> cross(const Vec<DIM>& other) const noexcept {
        std::cout << "[API ERROR]: No specialization of a cross product for Vec<" << DIM << ">\n";
        assert(false);
    }

    Vec<DIM>& normalize() noexcept {
        const auto len = length();
        for (unsigned int i = 0; i < DIM; i++) d[i] /= len;
        return *this;
    }

    Vec<DIM> normalized() const noexcept {
        Vec<DIM> res = *this;
        return res.normalize();
    }

    Vec<DIM> negated() const noexcept {
        Vec<DIM> res;
        for (unsigned int i = 0; i < DIM; i++) res[i] = -d[i];
        return res;
    }

    bool not_zero() const noexcept {
        static constexpr float e = 0.0000001f;
        for (unsigned int i = 0; i < DIM; i++) {
            if (std::abs(d[i]) < e) return false;
        }
        return true;
    }

    float length() const noexcept {
        return std::sqrt(length_sqr());
    }

    float length_sqr() const noexcept {
        float sum = 0.0f;
        for (unsigned int i = 0; i < DIM; i++) sum += d[i] * d[i];
        return sum;
    }
};

template<>
Vec<3> Vec<3>::cross(const Vec<3>& other) const noexcept {
    std::cout << "yes" << '\n';
    return Vec<3>{
        d[1] * other[2] - d[2] * other[1],
        d[2] * other[0] - d[0] * other[2],
        d[0] * other[1] - d[1] * other[0]
    };
}

template<unsigned int DIM>
std::ostream& operator<<(std::ostream& stream, const Vec<DIM>& vector) noexcept {
    stream << '[';
    for (unsigned int i = 1; i < DIM; i++) stream << vector[i - 1] << ',';
    stream << vector[DIM - 1];
    stream << ']';
    return stream;
}


#endif
