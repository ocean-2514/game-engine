#include "Graphics/VertexLayout.h"

namespace eng {

namespace {
    uint32_t GetTypeSize(VertexDataType type) {
        switch (type)
        {
        case VertexDataType::Float:
        case VertexDataType::Int:
        case VertexDataType::UInt:
            return 4;
        case VertexDataType::Byte:
        case VertexDataType::UByte:
            return 1;
        
        default:
            return 0;
        }
    }
}
    

void VertexLayout::Populate() {
    uint32_t offset = 0;
    for (auto& element : elements) {
        element.offset = offset;
        uint32_t elementSize = element.size * GetTypeSize(element.type);
        offset += elementSize;
    }
    stride = offset;
}

void VertexLayout::PopulateOffset() {
    uint32_t offset = 0;
    for (auto& element : elements) {
        element.offset = offset;
        offset += element.size * GetTypeSize(element.type);
    }
}

void VertexLayout::PopulateStride() {
    stride = 0;
    for (auto& element : elements) {
        stride += element.size * GetTypeSize(element.type);
    }
}




} // namespace eng
