#ifndef O_COMPONENT
#define O_COMPONENT

#include <cstddef>
#include <type_traits>

namespace eng {

class GameObject;
class RenderQueue;

class Component
{
public:
    using TypeId = std::size_t;

    Component(const Component& other) = delete;
    Component(Component&& other) = delete;
    Component& operator=(const Component& other) = delete;
    Component& operator=(Component&& other) = delete;
    virtual ~Component() = default;


    GameObject* GetOwner();
    const GameObject* GetOwner() const;
    void MarkForDestroy();
    bool IsAlive() const;
    virtual TypeId GetTypeId() const noexcept = 0;

    template<typename T>
    static TypeId GetStaticTypeId() noexcept {
        static_assert(std::is_base_of_v<Component, T>,
            "T must derive from Component");
        static const TypeId typeId = AcquireTypeId();
        return typeId;
    }

protected:
    GameObject* m_owner = nullptr;
    bool m_isAlive = true;
    
    Component() = default;
    virtual void OnUpdate(float deltaTime) {}
    virtual void OnRender(RenderQueue& queue) {}
    
private:
    friend class GameObject;

    static TypeId AcquireTypeId() noexcept;
};

#define ENG_COMPONENT_TYPE(ComponentClass) \
public: \
    TypeId GetTypeId() const noexcept override {                 \
        return Component::GetStaticTypeId<ComponentClass>();     \
    }





} // namespace eng



#endif // O_COMPONENT
