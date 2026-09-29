#ifndef O_MODEL_FACTORY
#define O_MODEL_FACTORY

#include "eng.h"

class ModelFactory
{
public:
    static std::shared_ptr<eng::Model> CreatePlayerModel();
    static std::shared_ptr<eng::AnimatorController> CreatePlayerAnimatorController();

private:
    static std::shared_ptr<eng::AnimationClip> CreateWalkingAnimationClip();
    static std::shared_ptr<eng::AnimationClip> CreateRunningAnimationClip();
    static std::shared_ptr<eng::AnimationClip> CreateJumpingAnimationClip();
    static std::shared_ptr<eng::AnimationClip> CreateIdleAnimationClip();
};


#endif // O_MODEL_FACTORY