#pragma once
#include <vector>
#include <utility>
#include <GLFW/glfw3.h>
#include "glm_mini.hpp"

#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE 0x812F
#endif

class PolygonWithTexture {
public:
    std::vector<float> vertices;
    glm::vec3 color;
    glm::mat4 model;
    GLuint texture;

    PolygonWithTexture(const std::vector<glm::vec3>& v,
                       const std::vector<std::pair<float, float>>& t,
                       const glm::vec3& c = glm::vec3(1.0f, 1.0f, 1.0f))
        : color(c), model(glm::mat4(1.0f)), texture(0)
    {
        for (size_t i = 0; i < v.size(); ++i) {
            vertices.push_back(v[i].x);
            vertices.push_back(v[i].y);
            vertices.push_back(v[i].z);

            if (i < t.size()) {
                vertices.push_back(t[i].first);
                vertices.push_back(t[i].second);
            } else {
                vertices.push_back(0.0f);
                vertices.push_back(0.0f);
            }
        }
    }

    void transformation(const glm::mat4& mat) {
        model = mat;
    }

    void setTexture(GLuint texID) {
        texture = texID;
    }

    void draw() const {
        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glMultMatrixf(glm::value_ptr(model));

        if (texture != 0) {
            glEnable(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D, texture);
        } else {
            glDisable(GL_TEXTURE_2D);
        }

        glColor3f(color.x, color.y, color.z);

        size_t count = vertices.size() / 5;
        glBegin(GL_TRIANGLE_FAN);
        for (size_t i = 0; i < count; ++i) {
            size_t idx = i * 5;
            if (texture != 0) {
                glTexCoord2f(vertices[idx + 3], vertices[idx + 4]);
            }
            glVertex3f(vertices[idx + 0], vertices[idx + 1], vertices[idx + 2]);
        }
        glEnd();

        if (texture != 0) {
            glBindTexture(GL_TEXTURE_2D, 0);
            glDisable(GL_TEXTURE_2D);
        }

        glPopMatrix();
    }
};