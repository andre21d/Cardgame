#pragma once
#include <cmath>
#include <algorithm>

namespace glm {

constexpr float PI = 3.14159265358979323846f;

inline float radians(float deg) {
    return deg * (PI / 180.0f);
}

struct vec3 {
    float x, y, z;
    vec3() : x(0.0f), y(0.0f), z(0.0f) {}
    explicit vec3(float s) : x(s), y(s), z(s) {}
    vec3(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}

    vec3(const vec3&) = default;
    vec3(vec3&&) = default;
    vec3& operator=(const vec3&) = default;
    vec3& operator=(vec3&&) = default;

    vec3 operator+(const vec3& o) const { return vec3(x + o.x, y + o.y, z + o.z); }
    vec3 operator-(const vec3& o) const { return vec3(x - o.x, y - o.y, z - o.z); }
    vec3 operator*(float s) const { return vec3(x * s, y * s, z * s); }
};

inline vec3 operator*(float s, const vec3& v) {
    return vec3(v.x * s, v.y * s, v.z * s);
}

inline float dot(const vec3& a, const vec3& b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

inline vec3 cross(const vec3& a, const vec3& b) {
    return vec3(
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    );
}

inline vec3 normalize(const vec3& v) {
    float len = std::sqrt(dot(v, v));
    if (len <= 1e-6f) return vec3(0.0f);
    return vec3(v.x / len, v.y / len, v.z / len);
}

inline vec3 mix(const vec3& a, const vec3& b, float t) {
    return vec3(
        a.x + (b.x - a.x) * t,
        a.y + (b.y - a.y) * t,
        a.z + (b.z - a.z) * t
    );
}

inline float mix(float a, float b, float t) {
    return a + (b - a) * t;
}

struct mat4 {
    float m[16];

    mat4() {
        for (int i = 0; i < 16; ++i) m[i] = 0.0f;
    }

    explicit mat4(float diag) {
        for (int i = 0; i < 16; ++i) m[i] = 0.0f;
        m[0] = m[5] = m[10] = m[15] = diag;
    }

    mat4 operator*(const mat4& rhs) const {
        mat4 out(0.0f);
        for (int col = 0; col < 4; ++col) {
            for (int row = 0; row < 4; ++row) {
                float sum = 0.0f;
                for (int k = 0; k < 4; ++k) {
                    sum += m[k * 4 + row] * rhs.m[col * 4 + k];
                }
                out.m[col * 4 + row] = sum;
            }
        }
        return out;
    }
};

inline const float* value_ptr(const mat4& mat) {
    return mat.m;
}

inline mat4 translate(const mat4& base, const vec3& v) {
    mat4 t(1.0f);
    t.m[12] = v.x;
    t.m[13] = v.y;
    t.m[14] = v.z;
    return base * t;
}

inline mat4 rotate(const mat4& base, float angleRad, const vec3& axis) {
    vec3 a = normalize(axis);
    float c = std::cos(angleRad);
    float s = std::sin(angleRad);
    float omc = 1.0f - c;

    mat4 r(1.0f);
    r.m[0] = c + a.x * a.x * omc;
    r.m[1] = a.y * a.x * omc + a.z * s;
    r.m[2] = a.z * a.x * omc - a.y * s;

    r.m[4] = a.x * a.y * omc - a.z * s;
    r.m[5] = c + a.y * a.y * omc;
    r.m[6] = a.z * a.y * omc + a.x * s;

    r.m[8]  = a.x * a.z * omc + a.y * s;
    r.m[9]  = a.y * a.z * omc - a.x * s;
    r.m[10] = c + a.z * a.z * omc;

    return base * r;
}

inline mat4 perspective(float fovyRad, float aspect, float zNear, float zFar) {
    float tanHalfFovy = std::tan(fovyRad * 0.5f);
    mat4 out(0.0f);
    out.m[0] = 1.0f / (aspect * tanHalfFovy);
    out.m[5] = 1.0f / tanHalfFovy;
    out.m[10] = -(zFar + zNear) / (zFar - zNear);
    out.m[11] = -1.0f;
    out.m[14] = -(2.0f * zFar * zNear) / (zFar - zNear);
    return out;
}

inline mat4 lookAt(const vec3& eye, const vec3& center, const vec3& up) {
    vec3 f = normalize(center - eye);
    vec3 s = normalize(cross(f, up));
    vec3 u = cross(s, f);

    mat4 out(1.0f);
    out.m[0] = s.x;
    out.m[4] = s.y;
    out.m[8] = s.z;

    out.m[1] = u.x;
    out.m[5] = u.y;
    out.m[9] = u.z;

    out.m[2]  = -f.x;
    out.m[6]  = -f.y;
    out.m[10] = -f.z;

    out.m[12] = -dot(s, eye);
    out.m[13] = -dot(u, eye);
    out.m[14] =  dot(f, eye);
    return out;
}

} // namespace glm