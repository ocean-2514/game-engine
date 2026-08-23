#ifndef O_TEXTURE
#define O_TEXTURE

#include "Renderer/TextureDesc.h"

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

protected:
    Texture() = default;

    int m_width = 0;
    int m_height = 0;
    TextureFormat m_format = TextureFormat::RGBA8;
};





} // namespace eng


#endif // O_TEXTURE
