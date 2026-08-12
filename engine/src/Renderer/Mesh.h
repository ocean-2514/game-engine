#ifndef O_MESH
#define O_MESH

#include "Graphics/VertexLayout.h"

namespace eng {
    
class Mesh {
public:
    virtual ~Mesh() = default;

    virtual void Bind() const = 0;
    virtual void Draw() const = 0;
    virtual bool IsValid() const = 0;

protected:
    VertexLayout m_vertexLayout;
};



} // namespace eng



#endif
