#ifndef O_MODEL_FACTORY
#define O_MODEL_FACTORY

#include "eng.h"

class ModelFactory
{
public:
    static std::shared_ptr<eng::Model> CreatePlayerModel();

private:
    static std::shared_ptr<eng::AnimationClip> CreateAnimationClip();
};


#endif // O_MODEL_FACTORY