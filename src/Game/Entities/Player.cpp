#include "Player.h"

PlayerMoveState Player::GetMoveState() const {
    return m_moveState;
}

void Player::SetMoveState(PlayerMoveState state) {
    m_moveState = state;
}
