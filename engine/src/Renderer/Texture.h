#ifndef O_TEXTURE
#define O_TEXTURE

#include "Renderer/TextureDesc.h"
#include <cstddef>

namespace eng
{
    

class Texture
{
public:
    virtual ~Texture() = default;

    Texture(const Texture&) = delete;
    Texture(Texture&&) = delete;
    Texture& operator=(const Texture&) = delete;
    Texture& operator=(Texture&&) = delete;

    int GetWidth() const;
    int GetHeight() const;
    TextureFormat GetFormat() const;
    bool HasAlphaChannel() const;
    virtual bool IsValid() const = 0;
    // Tightly packed rows, in the texture's existing format. Copies data before
    // returning; false leaves callers free to retry (e.g. a dirty glyph page).
    virtual bool Update(uint32_t x, uint32_t y, uint32_t width, uint32_t height,
        const void* pixels, std::size_t byteCount) = 0;

protected:
    Texture() = default;

    int m_width = 0;
    int m_height = 0;
    TextureFormat m_format = TextureFormat::RGBA8;
};





} // namespace eng


#endif // O_TEXTURE
