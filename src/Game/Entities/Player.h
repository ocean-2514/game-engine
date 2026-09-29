#ifndef O_PLAYER
#define O_PLAYER

enum class PlayerMoveState {
    Idle, Walking, Running, Jumping
};

class Player
{
public:

    PlayerMoveState GetMoveState() const;
    void SetMoveState(PlayerMoveState state);
    float GetSpeed() const;
    void SetSpeed(float speed);

    bool IsMoving() const;

private:
    PlayerMoveState m_moveState = PlayerMoveState::Idle;
    float m_speed = 2.0f;
};

#endif // O_PLAYER