#include <glad/gl.h>

#include "AxiomEngine/rendering/Mesh.h"

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

namespace axiom::rendering {

namespace {

struct Vertex {
    glm::vec3 position{};
    glm::vec3 normal{};
};

using Vertices = std::vector<Vertex>;
using Indices = std::vector<std::uint32_t>;

constexpr float HalfExtent = 0.5F;
constexpr std::uint32_t CircleSegments = 40U;
constexpr std::uint32_t SphereSlices = 32U;
constexpr std::uint32_t SphereStacks = 20U;

void AddTriangle(Vertices& vertices, Indices& indices, const glm::vec3& a, const glm::vec3& b,
                 const glm::vec3& c) {
    const glm::vec3 normal = glm::normalize(glm::cross(b - a, c - a));
    const auto first = static_cast<std::uint32_t>(vertices.size());
    vertices.push_back({a, normal});
    vertices.push_back({b, normal});
    vertices.push_back({c, normal});
    indices.insert(indices.end(), {first, first + 1U, first + 2U});
}

void AddOutwardTriangle(Vertices& vertices, Indices& indices, glm::vec3 a, glm::vec3 b, glm::vec3 c,
                        const glm::vec3& outwardHint) {
    const glm::vec3 normal = glm::cross(b - a, c - a);
    if (glm::dot(normal, outwardHint) < 0.0F) {
        std::swap(b, c);
    }
    AddTriangle(vertices, indices, a, b, c);
}

void AddQuad(Vertices& vertices, Indices& indices, const glm::vec3& a, const glm::vec3& b,
             const glm::vec3& c, const glm::vec3& d) {
    AddTriangle(vertices, indices, a, b, c);
    AddTriangle(vertices, indices, a, c, d);
}

void BuildCube(Vertices& vertices, Indices& indices) {
    constexpr float h = HalfExtent;
    AddQuad(vertices, indices, {-h, -h, h}, {h, -h, h}, {h, h, h}, {-h, h, h});
    AddQuad(vertices, indices, {h, -h, -h}, {-h, -h, -h}, {-h, h, -h}, {h, h, -h});
    AddQuad(vertices, indices, {h, -h, h}, {h, -h, -h}, {h, h, -h}, {h, h, h});
    AddQuad(vertices, indices, {-h, -h, -h}, {-h, -h, h}, {-h, h, h}, {-h, h, -h});
    AddQuad(vertices, indices, {-h, h, h}, {h, h, h}, {h, h, -h}, {-h, h, -h});
    AddQuad(vertices, indices, {-h, -h, -h}, {h, -h, -h}, {h, -h, h}, {-h, -h, h});
}

void BuildCircle(Vertices& vertices, Indices& indices) {
    vertices.push_back({{0.0F, 0.0F, 0.0F}, {0.0F, 0.0F, 1.0F}});
    for (std::uint32_t segment = 0; segment <= CircleSegments; ++segment) {
        const float angle = glm::two_pi<float>() * static_cast<float>(segment) /
                            static_cast<float>(CircleSegments);
        vertices.push_back({{HalfExtent * std::cos(angle), HalfExtent * std::sin(angle), 0.0F},
                            {0.0F, 0.0F, 1.0F}});
    }
    for (std::uint32_t segment = 0; segment < CircleSegments; ++segment) {
        indices.insert(indices.end(), {0U, segment + 1U, segment + 2U});
    }
}

void BuildPyramid(Vertices& vertices, Indices& indices) {
    constexpr float h = HalfExtent;
    constexpr glm::vec3 apex{0.0F, h, 0.0F};
    constexpr std::array<glm::vec3, 4> base{{{-h, -h, -h}, {h, -h, -h}, {h, -h, h}, {-h, -h, h}}};
    constexpr glm::vec3 baseCenter{0.0F, -h, 0.0F};

    for (std::size_t side = 0; side < base.size(); ++side) {
        const glm::vec3& first = base[side];
        const glm::vec3& second = base[(side + 1U) % base.size()];
        const glm::vec3 hint{(first.x + second.x) * 0.5F, 0.25F, (first.z + second.z) * 0.5F};
        AddOutwardTriangle(vertices, indices, first, second, apex, hint);
        AddTriangle(vertices, indices, baseCenter, first, second);
    }
}

void BuildCone(Vertices& vertices, Indices& indices) {
    constexpr float bottomY = -HalfExtent;
    constexpr float apexY = HalfExtent;
    constexpr glm::vec3 apex{0.0F, apexY, 0.0F};

    for (std::uint32_t segment = 0; segment < CircleSegments; ++segment) {
        const float angle0 = glm::two_pi<float>() * static_cast<float>(segment) /
                             static_cast<float>(CircleSegments);
        const float angle1 = glm::two_pi<float>() * static_cast<float>(segment + 1U) /
                             static_cast<float>(CircleSegments);
        const glm::vec3 first{HalfExtent * std::cos(angle0), bottomY, HalfExtent * std::sin(angle0)};
        const glm::vec3 second{HalfExtent * std::cos(angle1), bottomY, HalfExtent * std::sin(angle1)};
        const glm::vec3 sideHint{std::cos((angle0 + angle1) * 0.5F), 0.5F,
                                 std::sin((angle0 + angle1) * 0.5F)};
        AddOutwardTriangle(vertices, indices, first, apex, second, sideHint);
        AddTriangle(vertices, indices, {0.0F, bottomY, 0.0F}, first, second);
    }
}

void BuildCylinder(Vertices& vertices, Indices& indices) {
    constexpr float bottomY = -HalfExtent;
    constexpr float topY = HalfExtent;

    for (std::uint32_t segment = 0; segment < CircleSegments; ++segment) {
        const float angle0 = glm::two_pi<float>() * static_cast<float>(segment) /
                             static_cast<float>(CircleSegments);
        const float angle1 = glm::two_pi<float>() * static_cast<float>(segment + 1U) /
                             static_cast<float>(CircleSegments);
        const glm::vec3 radial0{std::cos(angle0), 0.0F, std::sin(angle0)};
        const glm::vec3 radial1{std::cos(angle1), 0.0F, std::sin(angle1)};
        const glm::vec3 bottom0{HalfExtent * radial0.x, bottomY, HalfExtent * radial0.z};
        const glm::vec3 top0{HalfExtent * radial0.x, topY, HalfExtent * radial0.z};
        const glm::vec3 bottom1{HalfExtent * radial1.x, bottomY, HalfExtent * radial1.z};
        const glm::vec3 top1{HalfExtent * radial1.x, topY, HalfExtent * radial1.z};
        const auto sideFirst = static_cast<std::uint32_t>(vertices.size());
        vertices.insert(vertices.end(), {{bottom0, radial0}, {top0, radial0}, {bottom1, radial1},
                                         {top1, radial1}});
        indices.insert(indices.end(), {sideFirst, sideFirst + 1U, sideFirst + 2U,
                                       sideFirst + 2U, sideFirst + 1U, sideFirst + 3U});

        const auto bottomCenter = static_cast<std::uint32_t>(vertices.size());
        vertices.push_back({{0.0F, bottomY, 0.0F}, {0.0F, -1.0F, 0.0F}});
        vertices.push_back({bottom0, {0.0F, -1.0F, 0.0F}});
        vertices.push_back({bottom1, {0.0F, -1.0F, 0.0F}});
        indices.insert(indices.end(), {bottomCenter, bottomCenter + 1U, bottomCenter + 2U});

        const auto topCenter = static_cast<std::uint32_t>(vertices.size());
        vertices.push_back({{0.0F, topY, 0.0F}, {0.0F, 1.0F, 0.0F}});
        vertices.push_back({top0, {0.0F, 1.0F, 0.0F}});
        vertices.push_back({top1, {0.0F, 1.0F, 0.0F}});
        indices.insert(indices.end(), {topCenter, topCenter + 2U, topCenter + 1U});
    }
}

glm::vec3 ParametricPosition(float latitude, float longitude, bool bean) {
    const float ringRadius = std::sin(latitude);
    const glm::vec3 spherePoint{ringRadius * std::cos(longitude), std::cos(latitude),
                                ringRadius * std::sin(longitude)};
    if (!bean) {
        return spherePoint * HalfExtent;
    }

    const float waist = 0.68F + 0.32F * std::abs(spherePoint.y);
    const float bend = 0.20F * (1.0F - spherePoint.y * spherePoint.y);
    return {HalfExtent * (spherePoint.x * waist + bend), HalfExtent * spherePoint.y,
            HalfExtent * spherePoint.z * waist};
}

void BuildUvShape(Vertices& vertices, Indices& indices, bool bean) {
    const std::uint32_t columns = SphereSlices + 1U;
    for (std::uint32_t stack = 0; stack <= SphereStacks; ++stack) {
        const float latitude = glm::pi<float>() * static_cast<float>(stack) /
                               static_cast<float>(SphereStacks);
        for (std::uint32_t slice = 0; slice <= SphereSlices; ++slice) {
            const float longitude = glm::two_pi<float>() * static_cast<float>(slice) /
                                    static_cast<float>(SphereSlices);
            vertices.push_back({ParametricPosition(latitude, longitude, bean), {0.0F, 0.0F, 0.0F}});
        }
    }

    for (std::uint32_t stack = 0; stack < SphereStacks; ++stack) {
        for (std::uint32_t slice = 0; slice < SphereSlices; ++slice) {
            const std::uint32_t a = stack * columns + slice;
            const std::uint32_t b = a + columns;
            const std::uint32_t c = a + 1U;
            const std::uint32_t d = b + 1U;
            indices.insert(indices.end(), {a, c, b, b, c, d});
        }
    }

    for (std::size_t i = 0; i + 2U < indices.size(); i += 3U) {
        const std::uint32_t a = indices[i];
        const std::uint32_t b = indices[i + 1U];
        const std::uint32_t c = indices[i + 2U];
        const glm::vec3 normal = glm::cross(vertices[b].position - vertices[a].position,
                                            vertices[c].position - vertices[a].position);
        vertices[a].normal += normal;
        vertices[b].normal += normal;
        vertices[c].normal += normal;
    }
    for (Vertex& vertex : vertices) {
        vertex.normal = glm::normalize(vertex.normal);
    }
}

} // namespace

Mesh::~Mesh() { Destroy(); }

bool Mesh::Build(PrimitiveShape shape) {
    Destroy();

    Vertices vertices{};
    Indices indices{};
    switch (shape) {
    case PrimitiveShape::Cube:
        BuildCube(vertices, indices);
        break;
    case PrimitiveShape::Sphere:
        BuildUvShape(vertices, indices, false);
        break;
    case PrimitiveShape::Circle:
        BuildCircle(vertices, indices);
        break;
    case PrimitiveShape::Pyramid:
        BuildPyramid(vertices, indices);
        break;
    case PrimitiveShape::Cone:
        BuildCone(vertices, indices);
        break;
    case PrimitiveShape::Cylinder:
        BuildCylinder(vertices, indices);
        break;
    case PrimitiveShape::Bean:
        BuildUvShape(vertices, indices, true);
        break;
    case PrimitiveShape::Count:
        return false;
    }

    if (vertices.empty() || indices.empty()) {
        return false;
    }

    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &vbo_);
    glGenBuffers(1, &ebo_);
    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertices.size() * sizeof(Vertex)), vertices.data(),
                 GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(indices.size() * sizeof(std::uint32_t)),
                 indices.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, static_cast<GLsizei>(sizeof(Vertex)),
                          reinterpret_cast<void*>(offsetof(Vertex, position)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, static_cast<GLsizei>(sizeof(Vertex)),
                          reinterpret_cast<void*>(offsetof(Vertex, normal)));
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    indexCount_ = static_cast<int>(indices.size());
    return vao_ != 0U && vbo_ != 0U && ebo_ != 0U;
}

void Mesh::Destroy() {
    if (ebo_ != 0U) {
        glDeleteBuffers(1, &ebo_);
        ebo_ = 0U;
    }
    if (vbo_ != 0U) {
        glDeleteBuffers(1, &vbo_);
        vbo_ = 0U;
    }
    if (vao_ != 0U) {
        glDeleteVertexArrays(1, &vao_);
        vao_ = 0U;
    }
    indexCount_ = 0;
}

void Mesh::Draw() const {
    if (vao_ == 0U || indexCount_ <= 0) {
        return;
    }
    glBindVertexArray(vao_);
    glDrawElements(GL_TRIANGLES, indexCount_, GL_UNSIGNED_INT, nullptr);
}

} // namespace axiom::rendering
