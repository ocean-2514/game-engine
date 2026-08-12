#ifndef O_TEST_OBJECT   
#define O_TEST_OBJECT   

#include "eng.h"

class TestObject : public eng::GameObject {
public:
    TestObject();
    
protected:
    void OnUpdate(float deltaTime) override;

public:
    std::shared_ptr<eng::Material> material;
    std::shared_ptr<eng::Mesh> mesh;
    std::shared_ptr<eng::ShaderProgram> shader;

private:
    float m_xoffset = 0.0f;
    float m_yoffset = 0.0f;
    float m_moveSpeed = 0.5f;
};

#endif