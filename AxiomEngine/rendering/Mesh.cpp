#include "AxiomEngine/rendering/Mesh.h"

#include <glad/gl.h>

namespace axiom::rendering {

Mesh::~Mesh() {
    if (ebo_ != 0U) glDeleteBuffers(1, &ebo_);
    if (vbo_ != 0U) glDeleteBuffers(1, &vbo_);
    if (vao_ != 0U) glDeleteVertexArrays(1, &vao_);
}

void Mesh::BuildCube() {
    constexpr float vertices[] = {
        -0.5F, -0.5F, -0.5F, 0.F, 0.F, -1.F, 0.5F, -0.5F, -0.5F, 0.F, 0.F, -1.F, 0.5F, 0.5F, -0.5F, 0.F, 0.F, -1.F, -0.5F, 0.5F, -0.5F, 0.F, 0.F, -1.F,
        -0.5F, -0.5F, 0.5F, 0.F, 0.F, 1.F, 0.5F, -0.5F, 0.5F, 0.F, 0.F, 1.F, 0.5F, 0.5F, 0.5F, 0.F, 0.F, 1.F, -0.5F, 0.5F, 0.5F, 0.F, 0.F, 1.F,
    };
    constexpr unsigned int indices[] = {
        0, 1, 2, 2, 3, 0, 4, 5, 6, 6, 7, 4, 0, 4, 7, 7, 3, 0,
        1, 5, 6, 6, 2, 1, 3, 2, 6, 6, 7, 3, 0, 1, 5, 5, 4, 0,
    };

    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &vbo_);
    glGenBuffers(1, &ebo_);

    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), nullptr);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), reinterpret_cast<void*>(3 * sizeof(float)));

    indexCount_ = static_cast<int>(sizeof(indices) / sizeof(indices[0]));
}

void Mesh::Draw() const {
    glBindVertexArray(vao_);
    glDrawElements(GL_TRIANGLES, indexCount_, GL_UNSIGNED_INT, nullptr);
}

} // namespace axiom::rendering
