#ifndef O_PLAYER
#define O_PLAYER

enum class PlayerMoveState {
    Idle, Walking
};

class Player
{
public:

    PlayerMoveState GetMoveState() const;
    void SetMoveState(PlayerMoveState state);

private:
    PlayerMoveState m_moveState = PlayerMoveState::Idle;
};

#endif // O_PLAYER