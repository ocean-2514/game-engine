#ifndef O_PHYSICS_WORLD
#define O_PHYSICS_WORLD

#include "Physics/PhysicsTypes.h"

#include <memory>

namespace eng
{
    
class PhysicsWorld
{
public:
    explicit PhysicsWorld(const PhysicsWorldDesc& desc);
    ~PhysicsWorld();

    PhysicsWorld(const PhysicsWorld& other) = delete;
    PhysicsWorld& operator=(const PhysicsWorld& other) = delete;
    PhysicsWorld(PhysicsWorld&& other) = delete;
    PhysicsWorld& operator=(PhysicsWorld&& other) = delete;

    void Simulate(float fixedDeltaTime);
    void SetGravity(const glm::vec3& gravity);
    glm::vec3 GetGravity() const;
    const PhysicsWorldDesc& GetDesc() const;
    bool IsValid() const;

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
    PhysicsWorldDesc m_desc;
};

    
} // namespace eng


#endif // O_PHYSICS_WORLD
