#ifndef O_TEST_OBJECT   
#define O_TEST_OBJECT   

#include "eng.h"

class TestObject : public eng::GameObject {
public:
    enum class Shape { Cube, Sphere, Plane };

    TestObject(
        Shape shape,
        eng::AssetManager& assetManager);
    
protected:
    void OnUpdate(float deltaTime) override;

public:
    std::shared_ptr<eng::Material> material;
    std::shared_ptr<eng::Mesh> mesh;

private:
    Shape m_shape;
};

#endif
