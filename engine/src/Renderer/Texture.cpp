#include "Renderer/Texture.h"

namespace eng
{
    

int Texture::GetWidth() const {
    return m_width;
}

int Texture::GetHeight() const {
    return m_height;
}

TextureFormat Texture::GetFormat() const {
    return m_format;
}

bool Texture::HasAlphaChannel() const {
    return m_format == TextureFormat::RGBA8 ||
        m_format == TextureFormat::SRGBA8;
}


    
} // namespace eng
