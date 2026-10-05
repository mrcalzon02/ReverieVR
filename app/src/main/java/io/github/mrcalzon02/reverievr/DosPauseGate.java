package io.github.mrcalzon02.reverievr;

final class DosPauseGate {
    private boolean lifecyclePaused;
    private boolean overlayPaused;

    synchronized void setLifecyclePaused(boolean paused) {
        lifecyclePaused = paused;
    }

    synchronized void setOverlayPaused(boolean paused) {
        overlayPaused = paused;
    }

    synchronized boolean isPaused() {
        return lifecyclePaused || overlayPaused;
    }

    synchronized boolean isOverlayPaused() {
        return overlayPaused;
    }

    synchronized void clear() {
        lifecyclePaused = false;
        overlayPaused = false;
    }
}
