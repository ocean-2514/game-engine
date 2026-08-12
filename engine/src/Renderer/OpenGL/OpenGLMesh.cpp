#include "Renderer/OpenGL/OpenGLMesh.h"

#include <cstdint>
#include <iostream>
#include <limits>

namespace eng {

namespace {

GLenum ToOpenGLType(VertexDataType type) {
    switch (type) {
        case VertexDataType::Float: return GL_FLOAT;
        case VertexDataType::Int:   return GL_INT;
        case VertexDataType::UInt:  return GL_UNSIGNED_INT;
        case VertexDataType::Byte:  return GL_BYTE;
        case VertexDataType::UByte: return GL_UNSIGNED_BYTE;
    }
    return 0;
}

std::size_t VertexDataTypeSize(VertexDataType type) {
    switch (type) {
        case VertexDataType::Float: return sizeof(float);
        case VertexDataType::Int:   return sizeof(std::int32_t);
        case VertexDataType::UInt:  return sizeof(std::uint32_t);
        case VertexDataType::Byte:  return sizeof(std::int8_t);
        case VertexDataType::UByte: return sizeof(std::uint8_t);
    }
    return 0;
}

bool ValidateLayout(const VertexLayout& layout, std::size_t vertexDataSize) {
    if (layout.stride == 0 || layout.elements.empty()) {
        std::cout << "OpenGLMesh: vertex layout must have a non-zero stride and at least one element\n";
        return false;
    }
    if (vertexDataSize == 0 || vertexDataSize % layout.stride != 0) {
        std::cout << "OpenGLMesh: vertex data size is not divisible by the layout stride\n";
        return false;
    }

    for (const auto& element : layout.elements) {
        const std::size_t typeSize = VertexDataTypeSize(element.type);
        const std::size_t elementSize = typeSize * element.size;
        if (typeSize == 0 || element.size == 0 || element.size > 4 ||
            element.offset + elementSize > layout.stride ||
            (element.integer && element.type == VertexDataType::Float) ||
            (element.integer && element.normalized)) {
            std::cout << "OpenGLMesh: invalid vertex element at attribute "
                << element.index << '\n';
            return false;
        }
    }
    return true;
}

} // namespace

OpenGLMesh::OpenGLMesh(
    const VertexLayout& layout,
    const std::vector<float>& vertices,
    const std::vector<uint32_t>& indices) {
    m_vertexLayout = layout;
    const std::size_t vertexDataSize = vertices.size() * sizeof(float);
    if (!ValidateLayout(m_vertexLayout, vertexDataSize)) {
        return;
    }

    const std::size_t vertexCount = vertexDataSize / m_vertexLayout.stride;
    if (vertexCount > static_cast<std::size_t>(std::numeric_limits<GLsizei>::max()) ||
        indices.size() > static_cast<std::size_t>(std::numeric_limits<GLsizei>::max())) {
        std::cout << "OpenGLMesh: vertex or index count exceeds OpenGL draw limits\n";
        return;
    }

    glGenVertexArrays(1, &m_VAO);
    glGenBuffers(1, &m_VBO);
    if (!indices.empty()) {
        glGenBuffers(1, &m_EBO);
    }
    if (m_VAO == 0 || m_VBO == 0 || (!indices.empty() && m_EBO == 0)) {
        std::cout << "OpenGLMesh: failed to create VAO, VBO, or EBO\n";
        return;
    }

    glBindVertexArray(m_VAO);

    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
    glBufferData(
        GL_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(vertexDataSize),
        vertices.data(),
        GL_STATIC_DRAW);

    if (m_EBO != 0) {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_EBO);
        glBufferData(
            GL_ELEMENT_ARRAY_BUFFER,
            static_cast<GLsizeiptr>(indices.size() * sizeof(uint32_t)),
            indices.data(),
            GL_STATIC_DRAW);
    }

    for (const auto& element : m_vertexLayout.elements) {
        const GLenum type = ToOpenGLType(element.type);
        const void* offset = reinterpret_cast<const void*>(
            static_cast<std::uintptr_t>(element.offset));

        if (element.integer) {
            glVertexAttribIPointer(
                element.index,
                static_cast<GLint>(element.size),
                type,
                static_cast<GLsizei>(m_vertexLayout.stride),
                offset);
        } else {
            glVertexAttribPointer(
                element.index,
                static_cast<GLint>(element.size),
                type,
                element.normalized ? GL_TRUE : GL_FALSE,
                static_cast<GLsizei>(m_vertexLayout.stride),
                offset);
        }
        glEnableVertexAttribArray(element.index);
    }

    // The EBO binding is part of the VAO state. Unbind the VAO first so the
    // indexed mesh keeps its EBO association.
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    m_vertexCnt = vertexCount;
    m_indexCnt = indices.size();
}

OpenGLMesh::~OpenGLMesh() {
    if (m_VAO != 0) {
        glDeleteVertexArrays(1, &m_VAO);
    }
    if (m_VBO != 0) {
        glDeleteBuffers(1, &m_VBO);
    }
    if (m_EBO != 0) {
        glDeleteBuffers(1, &m_EBO);
    }
}

void OpenGLMesh::Bind() const {
    if (IsValid()) {
        glBindVertexArray(m_VAO);
    }
}

void OpenGLMesh::Draw() const {
    if (!IsValid()) {
        return;
    }

    if (m_indexCnt == 0) {
        glDrawArrays(m_drawMode, 0, static_cast<GLsizei>(m_vertexCnt));
    } else {
        glDrawElements(
            m_drawMode,
            static_cast<GLsizei>(m_indexCnt),
            GL_UNSIGNED_INT,
            nullptr);
    }
}

bool OpenGLMesh::IsValid() const {
    return m_VAO != 0 && m_VBO != 0 && m_vertexCnt != 0;
}

void OpenGLMesh::setDrawMode(GLenum mode) {
    m_drawMode = mode;
}

} // namespace eng
