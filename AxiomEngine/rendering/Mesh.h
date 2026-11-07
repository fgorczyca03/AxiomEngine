#pragma once

#include "AxiomEngine/rendering/Components.h"

namespace axiom::rendering {

// A built-in primitive's VAO and buffers; Build/Destroy must run while its GL context is current.
class Mesh {
  public:
    Mesh() = default;
    ~Mesh();

    bool Build(PrimitiveShape shape);
    void Destroy();
    void Draw() const;

  private:
    unsigned int vao_{0};
    unsigned int vbo_{0};
    unsigned int ebo_{0};
    int indexCount_{0};
};

} // namespace axiom::rendering
