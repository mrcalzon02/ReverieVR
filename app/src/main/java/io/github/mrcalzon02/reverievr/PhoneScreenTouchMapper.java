package io.github.mrcalzon02.reverievr;

/**
 * Geometry and gesture safety boundary for a future authorized Android
 * screen-mirror touch panel. Pure Java: no ability to inject Android input.
 */
final class PhoneScreenTouchMapper {
    static final class Point {
        final float x, y;
        Point(float x, float y) { this.x = x; this.y = y; }
    }

    private final float left, top, width, height;
    private final int screenWidth, screenHeight;
    private boolean granted;
    private boolean pressed;
    private Point last;

    PhoneScreenTouchMapper(float left, float top, float width, float height,
        int screenWidth, int screenHeight) {
        if (!Float.isFinite(left) || !Float.isFinite(top) ||
            !Float.isFinite(width) || !Float.isFinite(height) ||
            width <= 0f || height <= 0f ||
            screenWidth <= 0 || screenHeight <= 0)
            throw new IllegalArgumentException("Invalid panel/display bounds");
        this.left = left;
        this.top = top;
        this.width = width;
        this.height = height;
        this.screenWidth = screenWidth;
        this.screenHeight = screenHeight;
    }

    void setAuthorized(boolean authorized) {
        granted = authorized;
        if (!authorized) cancel();
    }

    boolean isAuthorized() { return granted; }

    Point map(float panelX, float panelY) {
        if (!Float.isFinite(panelX) || !Float.isFinite(panelY) ||
            panelX < left || panelY < top ||
            panelX > left + width || panelY > top + height)
            return null;
        float u = (panelX - left) / width;
        float v = (panelY - top) / height;
        return new Point(Math.min(screenWidth - 1f, u * screenWidth),
            Math.min(screenHeight - 1f, v * screenHeight));
    }

    Point begin(float panelX, float panelY) {
        if (!granted) return null;
        Point point = map(panelX, panelY);
        if (point == null) return null;
        pressed = true;
        last = point;
        return point;
    }

    Point move(float panelX, float panelY) {
        if (!granted || !pressed) return null;
        Point point = map(panelX, panelY);
        // Leaving the live surface ends the gesture; never drag across
        // unrelated VR controls or out-of-bounds Android coordinates.
        if (point == null) { cancel(); return null; }
        last = point;
        return point;
    }

    Point end() {
        Point result = granted && pressed ? last : null;
        cancel();
        return result;
    }

    void cancel() { pressed = false; last = null; }
    boolean isPressed() { return pressed; }
}
