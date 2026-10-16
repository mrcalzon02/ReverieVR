package io.github.mrcalzon02.reverievr;

/**
 * A controller must report a fully released packet after reconnect/switch
 * before it can deliver any new game/menu press. No fake click or UI modal.
 */
final class ControllerResumeGate {
    private boolean awaitingNeutral = true;

    synchronized void requireNeutral() {
        awaitingNeutral = true;
    }

    synchronized boolean accept(ControllerSnapshot snapshot) {
        if (snapshot == null) return false;
        if (!awaitingNeutral) return true;
        if (snapshot.touching || snapshot.touchpadPressed
            || snapshot.menuPressed || snapshot.homePressed
            || snapshot.volumeUpPressed || snapshot.volumeDownPressed) {
            return false;
        }
        awaitingNeutral = false;
        return true;
    }
}
