#ifndef O_VERTEX_LAYOUT
#define O_VERTEX_LAYOUT

#include <vector>
#include <cstdint>

namespace eng {

enum class VertexDataType {
    Float,
    Int,
    UInt,
    Byte,
    UByte
};

struct VertexElement {
    uint32_t index;
    uint32_t size;
    uint32_t offset = 0;
    VertexDataType type = VertexDataType::Float;
    bool normalized = false;
    bool integer = false;
};

struct VertexLayout {
    std::vector<VertexElement> elements;
    uint32_t stride = 0;    //size of a vertex

    //populate the offset of each element and stride of a layout
    //assuming that vertex attribute data is compactly arranged
    void Populate();
    //assuming that vertex attribute data is compactly arranged
    void PopulateOffset();
    //assuming that vertex attribute data is compactly arranged
    void PopulateStride();
};



    
} // namespace eng



#endif
