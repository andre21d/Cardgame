#include <vector>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

class Polygon {
public:
    std::vector<glm::vec3> vertices;
    glm::vec3 color;
    glm::mat4 modelMatrix = glm::mat4(1.0f);

    unsigned int VAO = 0, VBO = 0;

    Polygon(const std::vector<glm::vec3>& verts, const glm::vec3& col)
        : vertices(verts), color(col)
    {
        setupMesh();
    }

    ~Polygon() {
        if (VBO != 0) glDeleteBuffers(1, &VBO);
        if (VAO != 0) glDeleteVertexArrays(1, &VAO);
    }

    void transformation(const glm::mat4& mat) {
        modelMatrix = mat;
    }

    template <typename ShaderType>
    void draw(ShaderType& shader) {
        shader.use();
        shader.setMat4("model", modelMatrix);
        shader.setVec3("ourColor", color);

        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLE_FAN, 0, static_cast<GLsizei>(vertices.size()));
        glBindVertexArray(0);
    }

private:
    void setupMesh() {
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);

        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), vertices.data(), GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
        glEnableVertexAttribArray(0);

        glBindVertexArray(0);
    }
};