package io.github.mrcalzon02.reverievr.controller;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.util.AttributeSet;
import android.view.MotionEvent;
import android.view.View;

public final class ControllerTouchpadView extends View {
    interface Listener {
        void onTouchpadEvent(
            int action,
            float normalizedX,
            float normalizedY
        );
    }

    private final Paint paint = new Paint(Paint.ANTI_ALIAS_FLAG);
    private Listener listener;

    public ControllerTouchpadView(Context context) {
        super(context);
        initialize();
    }

    public ControllerTouchpadView(
        Context context,
        AttributeSet attrs
    ) {
        super(context, attrs);
        initialize();
    }

    public ControllerTouchpadView(
        Context context,
        AttributeSet attrs,
        int defStyleAttr
    ) {
        super(context, attrs, defStyleAttr);
        initialize();
    }

    void setListener(Listener listener) {
        this.listener = listener;
    }

    private void initialize() {
        setFocusable(true);
        setClickable(true);
        setBackgroundColor(0xff171a20);
    }

    @Override
    protected void onDraw(Canvas canvas) {
        super.onDraw(canvas);

        float width = getWidth();
        float height = getHeight();

        paint.setStyle(Paint.Style.STROKE);
        paint.setStrokeWidth(3.0f);
        paint.setColor(0xff38d6c8);
        canvas.drawRect(
            1.5f,
            1.5f,
            width - 1.5f,
            height - 1.5f,
            paint
        );

        paint.setColor(0xff30343c);
        paint.setStrokeWidth(1.0f);
        canvas.drawLine(
            width * 0.5f,
            0.0f,
            width * 0.5f,
            height,
            paint
        );
        canvas.drawLine(
            0.0f,
            height * 0.5f,
            width,
            height * 0.5f,
            paint
        );

        paint.setStyle(Paint.Style.FILL);
        paint.setColor(0xffaeb6c2);
        paint.setTextAlign(Paint.Align.CENTER);
        paint.setTextSize(
            Math.max(24.0f, Math.min(width, height) * 0.08f)
        );
        canvas.drawText(
            "TOUCHPAD",
            width * 0.5f,
            height * 0.52f,
            paint
        );
    }

    @Override
    public boolean onTouchEvent(MotionEvent event) {
        if (event == null) {
            return false;
        }

        float width = Math.max(1.0f, getWidth());
        float height = Math.max(1.0f, getHeight());
        float x = clamp01(event.getX() / width);
        float y = clamp01(event.getY() / height);

        Listener target = listener;
        if (target != null) {
            target.onTouchpadEvent(
                event.getActionMasked(),
                x,
                y
            );
        }

        if (event.getActionMasked() == MotionEvent.ACTION_UP) {
            performClick();
        }
        return true;
    }

    @Override
    public boolean performClick() {
        super.performClick();
        return true;
    }

    private static float clamp01(float value) {
        return Math.max(0.0f, Math.min(1.0f, value));
    }
}
