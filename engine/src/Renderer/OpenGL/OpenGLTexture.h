#ifndef O_OPENGL_TEXTURE
#define O_OPENGL_TEXTURE

#include "Renderer/Texture.h"
#include "Renderer/TextureDesc.h"
#include <cstdint>
#include <glad/glad.h>

namespace eng
{


class OpenGLTexture final : public Texture
{
public:
    OpenGLTexture(
        const TextureDesc& textureDesc,
        const SamplerDesc& samplerDesc,
        const void* pixels,
        std::size_t byteCount);
    ~OpenGLTexture() override;

    bool IsValid() const override;
    bool Update(uint32_t x, uint32_t y, uint32_t width, uint32_t height,
        const void* pixels, std::size_t byteCount) override;

private:
    void Bind(uint32_t unit) const;

    GLuint m_textureId = 0;
    bool m_generateMipmaps = false;

    friend class OpenGLShaderProgram;
};



} // namespace eng


#endif // O_OPENGL_TEXTURE
