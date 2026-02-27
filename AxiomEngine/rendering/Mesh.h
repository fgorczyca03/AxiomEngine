#pragma once

namespace axiom::rendering {

class Mesh {
  public:
    Mesh() = default;
    ~Mesh();

    void BuildCube();
    void Draw() const;

  private:
    unsigned int vao_{0};
    unsigned int vbo_{0};
    unsigned int ebo_{0};
    int indexCount_{0};
};

} // namespace axiom::rendering
