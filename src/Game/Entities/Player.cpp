#include "Player.h"

PlayerMoveState Player::GetMoveState() const {
    return m_moveState;
}

void Player::SetMoveState(PlayerMoveState state) {
    m_moveState = state;
}

float Player::GetSpeed() const {
    return m_speed;
}

void Player::SetSpeed(float speed) {
    m_speed = speed;
}

bool Player::IsMoving() const {
    return m_moveState == PlayerMoveState::Walking || 
        m_moveState == PlayerMoveState::Running;
}

