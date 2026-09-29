#ifndef O_CAMERA
#define O_CAMERA

#include "eng.h"

class Camera : public eng::GameObject {
public:
    Camera();

protected:
    void OnUpdate(float deltaTime) override;

};

#endif // O_CAMERA