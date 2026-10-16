#ifndef REVERIE_CLASSIC_PONG_DYNAMICS_H
#define REVERIE_CLASSIC_PONG_DYNAMICS_H

#include <algorithm>
#include <cmath>

namespace reverie { namespace classics { namespace pong {
// World coordinates / second; no wall-clock, allocation or per-eye effects.
constexpr float kServeHorizontalSpeed = 0.90f;
constexpr float kServeVerticalSpeed = 0.27f;
constexpr float kCpuPaddleSpeed = 0.72f;
constexpr float kMaxCapturedPaddleSpeed = 2.20f;
constexpr float kMinBallSpeed = 0.68f;
constexpr float kMaxBallSpeed = 1.75f;
constexpr float kPaddleHalfHeight = 0.27f;

struct Rebound { float vx, vy; };

inline float PaddleVelocity(float displacement, float seconds) {
    if (!std::isfinite(displacement) || !std::isfinite(seconds) || seconds <= 0.f)
        return 0.f;
    return std::clamp(displacement / seconds,
                      -kMaxCapturedPaddleSpeed, kMaxCapturedPaddleSpeed);
}

inline Rebound Bounce(float vx, float vy, float impact_offset,
                      float paddle_velocity, bool left_paddle) {
    // Magnitudes come from the incoming ball, not a constant-x-speed hack.
    const float incoming = std::clamp(std::hypot(vx, vy),
                                      kMinBallSpeed, kMaxBallSpeed);
    const float offset = std::clamp(impact_offset / kPaddleHalfHeight, -1.f, 1.f);
    const float swing = std::clamp(paddle_velocity / kMaxCapturedPaddleSpeed,
                                    -1.f, 1.f);
    const float incoming_vertical = std::clamp(vy / incoming, -1.f, 1.f);
    // Where the ball meets the paddle establishes a baseline angle; a moving
    // paddle contributes genuine tangential spin and can change its direction.
    float slope = std::clamp(0.62f * offset + 0.40f * swing +
                             0.28f * incoming_vertical, -0.83f, 0.83f);
    // A dead-center flat hit must not enter an endless horizontal rally.
    if (std::abs(slope) < 0.11f)
        slope = slope < 0.f ? -0.11f : 0.11f;
    // Swinging with the ball's vertical travel gives energy; against it
    // absorbs energy. A gentle default rally increase avoids sluggish starts.
    const float alignment = swing * (0.60f * offset +
                                      0.40f * incoming_vertical);
    const float speed = std::clamp(incoming + 0.065f + 0.30f * alignment,
                                    kMinBallSpeed, kMaxBallSpeed);
    const float horizontal = speed * std::sqrt(std::max(0.f, 1.f - slope * slope));
    return {left_paddle ? horizontal : -horizontal, speed * slope};
}

inline float CpuTarget(float ball_y, float ball_vy, float ball_vx,
                       bool moving, int score, int opponent) {
    if (!moving || ball_vx <= 0.f) return 0.f; // Recover toward mid-court.
    // Modest, repeatable read error prevents perfect tracking without RNG.
    const int variation = (score * 3 + opponent * 5) % 5 - 2;
    return std::clamp(ball_y + std::clamp(ball_vy * 0.11f, -0.12f, 0.12f)
                            + static_cast<float>(variation) * 0.043f,
                      -0.78f, 0.78f);
}

}}} // namespace reverie::classics::pong
#endif
