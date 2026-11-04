#include "Renderer/OpenGL/OpenGLTexture.h"

#include <iostream>
#include <limits>

namespace eng {

namespace {

// Upload APIs borrow CPU bytes, regardless of the caller's pixel-unpack state.
struct PixelUnpackState {
    GLint alignment = 4, rowLength = 0, skipRows = 0, skipPixels = 0, buffer = 0;
    PixelUnpackState() {
        glGetIntegerv(GL_UNPACK_ALIGNMENT, &alignment);
        glGetIntegerv(GL_UNPACK_ROW_LENGTH, &rowLength);
        glGetIntegerv(GL_UNPACK_SKIP_ROWS, &skipRows);
        glGetIntegerv(GL_UNPACK_SKIP_PIXELS, &skipPixels);
        glGetIntegerv(GL_PIXEL_UNPACK_BUFFER_BINDING, &buffer);
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
        glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
        glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
    }
    ~PixelUnpackState() {
        glPixelStorei(GL_UNPACK_ALIGNMENT, alignment);
        glPixelStorei(GL_UNPACK_ROW_LENGTH, rowLength);
        glPixelStorei(GL_UNPACK_SKIP_ROWS, skipRows);
        glPixelStorei(GL_UNPACK_SKIP_PIXELS, skipPixels);
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, static_cast<GLuint>(buffer));
    }
};

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
    if (textureDesc.width == 0 || textureDesc.height == 0 ||
        format.internalFormat == 0 || pixels == nullptr ||
        textureDesc.width > static_cast<uint32_t>(std::numeric_limits<int>::max()) ||
        textureDesc.height > static_cast<uint32_t>(std::numeric_limits<int>::max()) ||
        textureDesc.width > std::numeric_limits<std::size_t>::max() / textureDesc.height / format.bytesPerPixel ||
        byteCount < static_cast<std::size_t>(textureDesc.width) * textureDesc.height * format.bytesPerPixel) {
        std::cout << "OpenGLTexture: invalid texture description or pixel data\n";
        return;
    }

    m_width = static_cast<int>(textureDesc.width);
    m_height = static_cast<int>(textureDesc.height);
    m_format = textureDesc.format;
    m_generateMipmaps = textureDesc.generateMipmaps;

    GLint previousActiveTexture = GL_TEXTURE0;
    GLint previousTextureBinding = 0;
    glGetIntegerv(GL_ACTIVE_TEXTURE, &previousActiveTexture);
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &previousTextureBinding);
    PixelUnpackState unpack;

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
    if (glGetError() != GL_NO_ERROR) {
        glDeleteTextures(1, &m_textureId);
        m_textureId = 0;
    }

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

bool OpenGLTexture::Update(uint32_t x, uint32_t y, uint32_t width, uint32_t height,
    const void* pixels, std::size_t byteCount) {
    if (!IsValid() || pixels == nullptr || width == 0 || height == 0) {
        return false;
    }
    const auto format = ToOpenGLFormat(m_format);
    if (x > static_cast<uint32_t>(m_width) || y > static_cast<uint32_t>(m_height)
        || width > static_cast<uint32_t>(m_width) - x
        || height > static_cast<uint32_t>(m_height) - y
        || format.bytesPerPixel == 0
        || width > std::numeric_limits<std::size_t>::max() / height / format.bytesPerPixel
        || byteCount < static_cast<std::size_t>(width) * height * format.bytesPerPixel)
        return false;
    GLint previousActiveTexture = GL_TEXTURE0;
    GLint previousTextureBinding = 0;
    glGetIntegerv(GL_ACTIVE_TEXTURE, &previousActiveTexture);
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &previousTextureBinding);

    PixelUnpackState unpack;
    glBindTexture(GL_TEXTURE_2D, m_textureId);
    glTexSubImage2D(
        GL_TEXTURE_2D,
        0,
        static_cast<GLint>(x),
        static_cast<GLint>(y),
        static_cast<GLsizei>(width),
        static_cast<GLsizei>(height),
        format.externalFormat,
        GL_UNSIGNED_BYTE,
        pixels);
    if (m_generateMipmaps) glGenerateMipmap(GL_TEXTURE_2D);
    // Conservatively report pending GL errors as failure; callers retain dirty data.
    const bool succeeded = glGetError() == GL_NO_ERROR;

    glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(previousTextureBinding));
    glActiveTexture(static_cast<GLenum>(previousActiveTexture));
    return succeeded;
}

void OpenGLTexture::Bind(uint32_t unit) const {
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, m_textureId);
}

} // namespace eng
