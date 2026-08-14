#ifndef O_OPENGL_MESH
#define O_OPENGL_MESH

#include "Renderer/Mesh.h"
#include <glad/glad.h>
#include <utility>

namespace eng {
    
class OpenGLMesh : public Mesh {
public:
    OpenGLMesh(const VertexLayout& layout, const std::vector<float>& vertices, const std::vector<uint32_t>& indices = {});
    ~OpenGLMesh() override;

    OpenGLMesh(const OpenGLMesh&) = delete;
    OpenGLMesh(OpenGLMesh&&) = delete;
    OpenGLMesh& operator=(const OpenGLMesh&) = delete;
    OpenGLMesh& operator=(OpenGLMesh&&) = delete;

    //you need to call Bind() before Draw()
    void Bind() const override;
    void Draw() const override;
    bool IsValid() const override;

    void setDrawMode(GLenum mode);

private:
    GLuint m_VAO = 0;
    GLuint m_VBO = 0;
    GLuint m_EBO = 0;
    GLenum m_drawMode = GL_TRIANGLES;

    std::size_t m_vertexCnt = 0;
    std::size_t m_indexCnt = 0;

};






} // namespace eng



#endif
