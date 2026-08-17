#include "Renderer/OpenGL/OpenGLTexture.h"

#include <iostream>

namespace eng {

namespace {

struct OpenGLFormat {
    GLint internalFormat;
    GLenum externalFormat;
    std::size_t bytesPerPixel;
};

OpenGLFormat ToOpenGLFormat(TextureFormat format) {
    switch (format) {
        case TextureFormat::R8:     return {GL_R8, GL_RED, 1};
        case TextureFormat::RG8:    return {GL_RG8, GL_RG, 2};
        case TextureFormat::RGB8:   return {GL_RGB8, GL_RGB, 3};
        case TextureFormat::RGBA8:  return {GL_RGBA8, GL_RGBA, 4};
        case TextureFormat::SRGB8:  return {GL_SRGB8, GL_RGB, 3};
        case TextureFormat::SRGBA8: return {GL_SRGB8_ALPHA8, GL_RGBA, 4};
    }
    return {0, 0, 0};
}

GLint ToOpenGLAddressMode(TextureAddressMode mode) {
    switch (mode) {
        case TextureAddressMode::Repeat:         return GL_REPEAT;
        case TextureAddressMode::ClampToEdge:    return GL_CLAMP_TO_EDGE;
        case TextureAddressMode::MirroredRepeat: return GL_MIRRORED_REPEAT;
    }
    return GL_REPEAT;
}

GLint ToOpenGLMagFilter(TextureFilter filter) {
    return filter == TextureFilter::Nearest ? GL_NEAREST : GL_LINEAR;
}

GLint ToOpenGLMinFilter(
    const SamplerDesc& sampler,
    bool hasMipmaps) {
    if (!hasMipmaps) {
        return ToOpenGLMagFilter(sampler.minFilter);
    }
    if (sampler.minFilter == TextureFilter::Nearest) {
        return sampler.mipFilter == TextureFilter::Nearest
            ? GL_NEAREST_MIPMAP_NEAREST
            : GL_NEAREST_MIPMAP_LINEAR;
    }
    return sampler.mipFilter == TextureFilter::Nearest
        ? GL_LINEAR_MIPMAP_NEAREST
        : GL_LINEAR_MIPMAP_LINEAR;
}

} // namespace

OpenGLTexture::OpenGLTexture(
    const TextureDesc& textureDesc,
    const SamplerDesc& samplerDesc,
    const void* pixels,
    std::size_t byteCount) {
    const OpenGLFormat format = ToOpenGLFormat(textureDesc.format);
    const std::size_t expectedByteCount =
        static_cast<std::size_t>(textureDesc.width) *
        static_cast<std::size_t>(textureDesc.height) *
        format.bytesPerPixel;
    if (textureDesc.width == 0 || textureDesc.height == 0 ||
        format.internalFormat == 0 || pixels == nullptr ||
        byteCount < expectedByteCount) {
        std::cout << "OpenGLTexture: invalid texture description or pixel data\n";
        return;
    }

    m_width = static_cast<int>(textureDesc.width);
    m_height = static_cast<int>(textureDesc.height);

    GLint previousActiveTexture = GL_TEXTURE0;
    GLint previousTextureBinding = 0;
    GLint previousUnpackAlignment = 4;
    glGetIntegerv(GL_ACTIVE_TEXTURE, &previousActiveTexture);
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &previousTextureBinding);
    glGetIntegerv(GL_UNPACK_ALIGNMENT, &previousUnpackAlignment);

    glGenTextures(1, &m_textureId);
    glBindTexture(GL_TEXTURE_2D, m_textureId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,
        ToOpenGLAddressMode(samplerDesc.addressU));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,
        ToOpenGLAddressMode(samplerDesc.addressV));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
        ToOpenGLMinFilter(samplerDesc, textureDesc.generateMipmaps));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
        ToOpenGLMagFilter(samplerDesc.magFilter));

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        format.internalFormat,
        m_width,
        m_height,
        0,
        format.externalFormat,
        GL_UNSIGNED_BYTE,
        pixels);
    if (textureDesc.generateMipmaps) {
        glGenerateMipmap(GL_TEXTURE_2D);
    }

    glPixelStorei(GL_UNPACK_ALIGNMENT, previousUnpackAlignment);
    glBindTexture(
        GL_TEXTURE_2D, static_cast<GLuint>(previousTextureBinding));
    glActiveTexture(static_cast<GLenum>(previousActiveTexture));
}

OpenGLTexture::~OpenGLTexture() {
    if (m_textureId != 0) {
        glDeleteTextures(1, &m_textureId);
        m_textureId = 0;
    }
}

bool OpenGLTexture::IsValid() const {
    return m_textureId != 0 && m_width > 0 && m_height > 0;
}

void OpenGLTexture::Bind(uint32_t unit) const {
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, m_textureId);
}

} // namespace eng
