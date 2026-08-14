#ifndef O_TEST_OBJECT   
#define O_TEST_OBJECT   

#include "eng.h"

class TestObject : public eng::GameObject {
public:
    enum class Shape { Cube, Sphere, Plane };

    TestObject(Shape shape = Shape::Cube,
        glm::vec3 color = {0.9f, 0.4f, 0.3f});
    
protected:
    void OnUpdate(float deltaTime) override;

public:
    std::shared_ptr<eng::Material> material;
    std::shared_ptr<eng::Mesh> mesh;
    std::shared_ptr<eng::ShaderProgram> shader;

private:
    Shape m_shape;
    glm::vec3 m_color;
};

#endif