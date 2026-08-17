#ifndef O_TEXTURE
#define O_TEXTURE

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
    virtual bool IsValid() const = 0;

protected:
    Texture() = default;

    int m_width = 0;
    int m_height = 0;
};





} // namespace eng


#endif // O_TEXTURE
