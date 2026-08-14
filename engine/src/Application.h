#ifndef O_APPLICATION
#define O_APPLICATION

#include "Renderer/RenderQueue.h"

namespace eng {

    
class Application {
public:
    virtual bool Init() = 0;
    //deltaTime in seconds
    virtual void Update(float deltaTime) = 0;
    virtual void Render(RenderQueue& queue) = 0;
    virtual void Destroy() = 0;

    void SetNeedsToBeClosed(bool value);
    bool NeedsToBeClosed() const;
    
    Application() = default;
    virtual ~Application() = default;

protected:
    bool m_needsToBeClosed = false;
};

    
}

#endif