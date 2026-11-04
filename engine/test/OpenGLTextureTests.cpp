// Exercise the actual OpenGLTexture implementation with GLAD function spies.
// No window or graphics driver is required by this test.
#include "Renderer/OpenGL/OpenGLTexture.h"
#include <iostream>
#include <stdexcept>

using namespace eng;
namespace {
void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}
GLint active = GL_TEXTURE3, binding = 77, alignment = 8, rowLength = 21;
GLint skipRows = 2, skipPixels = 3, unpackBuffer = 11;
int allocations = 0, updates = 0, mipmaps = 0;
GLenum nextError = GL_NO_ERROR;

void APIENTRY GetInteger(GLenum name, GLint* value) {
    switch (name) {
        case GL_ACTIVE_TEXTURE: *value = active; break;
        case GL_TEXTURE_BINDING_2D: *value = binding; break;
        case GL_UNPACK_ALIGNMENT: *value = alignment; break;
        case GL_UNPACK_ROW_LENGTH: *value = rowLength; break;
        case GL_UNPACK_SKIP_ROWS: *value = skipRows; break;
        case GL_UNPACK_SKIP_PIXELS: *value = skipPixels; break;
        case GL_PIXEL_UNPACK_BUFFER_BINDING: *value = unpackBuffer; break;
        default: throw std::runtime_error("unexpected GL state query");
    }
}
void APIENTRY PixelStore(GLenum name, GLint value) {
    switch (name) {
        case GL_UNPACK_ALIGNMENT: alignment = value; break;
        case GL_UNPACK_ROW_LENGTH: rowLength = value; break;
        case GL_UNPACK_SKIP_ROWS: skipRows = value; break;
        case GL_UNPACK_SKIP_PIXELS: skipPixels = value; break;
        default: throw std::runtime_error("unexpected pixel store");
    }
}
void CheckUpload() {
    Check(alignment == 1 && rowLength == 0 && skipRows == 0 && skipPixels == 0
        && unpackBuffer == 0, "CPU uploads must ignore external unpack state");
}
void CheckRestored() {
    Check(active == GL_TEXTURE3 && binding == 77 && alignment == 8 && rowLength == 21
        && skipRows == 2 && skipPixels == 3 && unpackBuffer == 11, "upload must restore GL state");
}
void APIENTRY BindBuffer(GLenum, GLuint value) { unpackBuffer = value; }
void APIENTRY BindTexture(GLenum, GLuint value) { binding = value; }
void APIENTRY ActiveTexture(GLenum value) { active = value; }
void APIENTRY GenTextures(GLsizei count, GLuint* ids) {
    for (GLsizei i = 0; i < count; ++i) ids[i] = 1234 + i;
}
void APIENTRY DeleteTextures(GLsizei, const GLuint*) {}
void APIENTRY TexParameter(GLenum, GLenum, GLint) {}
void APIENTRY TexImage(GLenum, GLint, GLint, GLsizei, GLsizei, GLint, GLenum, GLenum, const void*) {
    CheckUpload(); ++allocations;
}
void APIENTRY TexSubImage(GLenum, GLint, GLint, GLint, GLsizei width, GLsizei height,
    GLenum format, GLenum, const void*) {
    CheckUpload();
    Check(width == 3 && height == 2 && format == GL_RED, "R8 update shape and format");
    ++updates;
}
void APIENTRY GenerateMipmap(GLenum) { ++mipmaps; }
GLenum APIENTRY GetError() {
    auto error = nextError; nextError = GL_NO_ERROR; return error;
}
void InstallSpies() {
    glad_glGetIntegerv = GetInteger;
    glad_glPixelStorei = PixelStore;
    glad_glBindBuffer = BindBuffer;
    glad_glBindTexture = BindTexture;
    glad_glActiveTexture = ActiveTexture;
    glad_glGenTextures = GenTextures;
    glad_glDeleteTextures = DeleteTextures;
    glad_glTexParameteri = TexParameter;
    glad_glTexImage2D = TexImage;
    glad_glTexSubImage2D = TexSubImage;
    glad_glGenerateMipmap = GenerateMipmap;
    glad_glGetError = GetError;
}
}

int main() {
    try {
        InstallSpies();
        const uint8_t pixels[6] = {1,2,3,4,5,6};
        OpenGLTexture texture({3,2,TextureFormat::R8,false}, {}, pixels, sizeof(pixels));
        Check(texture.IsValid() && allocations == 1 && mipmaps == 0, "create R8 texture");
        CheckRestored();
        Check(texture.Update(0,0,3,2,pixels,sizeof(pixels)) && updates == 1, "odd-width R8 update");
        CheckRestored();
        Check(!texture.Update(0,0,3,2,pixels,5), "short buffer rejected");
        Check(!texture.Update(1,0,3,2,pixels,6), "out of bounds rejected");
        Check(!texture.Update(UINT32_MAX,0,3,2,pixels,6), "overflowing offset rejected");
        Check(!texture.Update(0,0,0,2,pixels,6) && !texture.Update(0,0,3,2,nullptr,6), "empty update rejected");
        Check(updates == 1, "invalid requests do not reach GL");
        CheckRestored();
        nextError = GL_OUT_OF_MEMORY;
        Check(!texture.Update(0,0,3,2,pixels,6), "GL error reported to caller");
        CheckRestored();
        OpenGLTexture withMipmaps({3,2,TextureFormat::R8,true}, {}, pixels,6);
        Check(withMipmaps.IsValid() && mipmaps == 1, "create mipmap chain");
        Check(withMipmaps.Update(0,0,3,2,pixels,6) && mipmaps == 2, "update refreshes mipmaps");
        CheckRestored();
        nextError = GL_OUT_OF_MEMORY;
        OpenGLTexture failed({3,2,TextureFormat::R8,false}, {}, pixels,6);
        Check(!failed.IsValid(), "failed allocation is not a valid texture");
        CheckRestored();
        std::cout << "OpenGL texture tests passed\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "OpenGL texture tests failed: " << e.what() << '\n';
        return 1;
    }
}
