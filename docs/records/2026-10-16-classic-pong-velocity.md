# Classic Pong — paddle-velocity energy transfer (2026-10-16)

**Scope:** Game-private native classic Pong physics/CPU adjustments only. No modifications to the VR Home shell, classic menu, controller bindings, ABI v1, Snake, Chutes and Ladders, or native GPU renderer.

## Cause
Previous `classic_games.h` served at (`0.55,0.25`) normalized units/second (speed ~0.604), tracked the incoming ball continuously with a CPU speed cap of 0.87 units/second, and applied only a small hit-offset adjustment to Y velocity after paddle contact. Hit motion supplied no kinetic energy. This made the CPU unusually hard to beat while preserving near-flat repetitive rallies.

## Updated behavior
- Serve is now (`0.90,0.27`), speed ~0.94, and resets to that speed after each point; sides/directions alternate deterministically.
- Player paddle velocity is measured after both the shell's normalized vertical trackpad axis and head-pointer position adjustments, using its location on the previous simulation step. Captured velocity is limited to 2.2 court units/second, protecting frame-rate spikes and fast pointer motion.
- Both human and CPU paddle collisions use `classic_pong_dynamics.h::Bounce`: contact offset, paddle vertical velocity and incoming ball Y angle determine outgoing direction. Moving with the ball's vertical travel accelerates it; moving against it absorbs energy. A modest default rally boost means even stationary rebounds speed up gradually. Resultant ball speed is capped to 0.68–1.75, rebound slope to 0.83, and a minimum angle avoids dead-flat exchanges. No random calls, allocations, shaders or UI.
- The CPU follows the ball toward its own side at at most 0.72 court units/second with deterministic small targeting error, and recovers to the center when the ball heads away. The AI is deliberately beatable at high returns; this is not adaptive difficulty.
- A paddle contact is only eligible as the ball crosses the +/-0.87 paddle plane from the court side; a ball already beyond a paddle plane cannot be caught retroactively on a later tick.
- First-to-11 scoring, mouse/head-look targeting, input profiles and persistent shell quick menu remain unchanged.

## Test evidence
The exact SHA1 Git blobs of new dynamics source and its fixture were reconstructed locally and tested using strict GCC and Clang C++17. Isolated tests cover swing alignment acceleration/inverse braking, left/right rebound direction, speed bounds, minimum center deflection, sustained rallies and deterministic CPU aiming. A separate C++ harness reproducing the revised Pong Step behavior passed both compilers for split versus unsplit update cadence, stroke effects, missed-paddle point award, serve reset, CPU speed limit and eleven-point win/restart. The full repository `classic_games_test.cpp` and Android/NDK build were **not executed** during this pass. The host check runner `scripts/verify-classic-pong.sh` can compile and run the isolated dynamics test and full native Classic Games test under available GCC/Clang. Device feel and acceptance on Galaxy S9/Daydream remain pending; do not label this APK device-accepted from source/tests alone.
