package io.github.mrcalzon02.reverievr;

import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.PorterDuff;
import android.graphics.Typeface;
import android.opengl.GLES20;
import android.opengl.GLUtils;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.FloatBuffer;

/** Head-locked, top-center stereo FPS label. It is independent of the battery HUD.
 * The one small texture is repainted only when the measured value changes. */
final class StereoFpsHud {
    private static final int WIDTH = 192;
    private static final int HEIGHT = 64;
    // Normalized within EACH Cardboard eye, after restoring its full viewport.
    private static final float LEFT = -0.20f;
    private static final float RIGHT = 0.20f;
    private static final float BOTTOM = 0.76f;
    private static final float TOP = 0.91f;
    private static final float[] IDENTITY = {
        1f, 0f, 0f, 0f,
        0f, 1f, 0f, 0f,
        0f, 0f, 1f, 0f,
        0f, 0f, 0f, 1f
    };

    private final FrameRateMeter meter = new FrameRateMeter();
    private final FloatBuffer vertices = allocate(new float[] {
        LEFT, BOTTOM, 0f, RIGHT, BOTTOM, 0f,
        LEFT, TOP, 0f, RIGHT, TOP, 0f
    });
    private final FloatBuffer uvs = allocate(new float[] {
        0f, 1f, 1f, 1f, 0f, 0f, 1f, 0f
    });

    private Bitmap bitmap;
    private Canvas canvas;
    private Paint paint;
    private int texture;
    private boolean textureAllocated;
    private boolean dirty = true;

    int getFps() { return meter.getFps(); }

    void recordFrame(long frameNanos) {
        if (meter.recordFrame(frameNanos)) dirty = true;
    }

    void onSurfaceCreated() {
        int[] handle = new int[1];
        GLES20.glGenTextures(1, handle, 0);
        texture = handle[0];
        GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, texture);
        GLES20.glTexParameteri(GLES20.GL_TEXTURE_2D,
            GLES20.GL_TEXTURE_MIN_FILTER, GLES20.GL_LINEAR);
        GLES20.glTexParameteri(GLES20.GL_TEXTURE_2D,
            GLES20.GL_TEXTURE_MAG_FILTER, GLES20.GL_LINEAR);
        GLES20.glTexParameteri(GLES20.GL_TEXTURE_2D,
            GLES20.GL_TEXTURE_WRAP_S, GLES20.GL_CLAMP_TO_EDGE);
        GLES20.glTexParameteri(GLES20.GL_TEXTURE_2D,
            GLES20.GL_TEXTURE_WRAP_T, GLES20.GL_CLAMP_TO_EDGE);
        textureAllocated = false;
        dirty = true;
    }

    void draw(int program, int positionHandle, int uvHandle,
              int matrixHandle, int textureHandle) {
        if (texture == 0 || program == 0) return;
        if (dirty) uploadIfChanged();
        GLES20.glDisable(GLES20.GL_DEPTH_TEST);
        GLES20.glUseProgram(program);
        vertices.position(0);
        GLES20.glVertexAttribPointer(positionHandle, 3,
            GLES20.GL_FLOAT, false, 0, vertices);
        GLES20.glEnableVertexAttribArray(positionHandle);
        uvs.position(0);
        GLES20.glVertexAttribPointer(uvHandle, 2,
            GLES20.GL_FLOAT, false, 0, uvs);
        GLES20.glEnableVertexAttribArray(uvHandle);
        GLES20.glUniformMatrix4fv(matrixHandle, 1, false, IDENTITY, 0);
        GLES20.glActiveTexture(GLES20.GL_TEXTURE0);
        GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, texture);
        GLES20.glUniform1i(textureHandle, 0);
        GLES20.glEnable(GLES20.GL_BLEND);
        GLES20.glBlendFunc(GLES20.GL_SRC_ALPHA,
            GLES20.GL_ONE_MINUS_SRC_ALPHA);
        GLES20.glDrawArrays(GLES20.GL_TRIANGLE_STRIP, 0, 4);
        GLES20.glDisable(GLES20.GL_BLEND);
        GLES20.glDisableVertexAttribArray(positionHandle);
        GLES20.glDisableVertexAttribArray(uvHandle);
    }

    private void uploadIfChanged() {
        if (bitmap == null || bitmap.isRecycled()) {
            bitmap = Bitmap.createBitmap(WIDTH, HEIGHT, Bitmap.Config.ARGB_8888);
            canvas = new Canvas(bitmap);
            paint = new Paint(Paint.ANTI_ALIAS_FLAG);
            paint.setTypeface(Typeface.create(Typeface.MONOSPACE, Typeface.BOLD));
        }
        canvas.drawColor(Color.TRANSPARENT, PorterDuff.Mode.CLEAR);
        paint.setStyle(Paint.Style.FILL);
        paint.setColor(Color.argb(202, 10, 14, 20));
        canvas.drawRoundRect(2f, 4f, 190f, 60f, 10f, 10f, paint);
        paint.setColor(bandColor(FrameRateMeter.bandFor(meter.getFps())));
        canvas.drawRoundRect(12f, 17f, 43f, 48f, 4f, 4f, paint);
        paint.setColor(Color.WHITE);
        paint.setTextAlign(Paint.Align.LEFT);
        paint.setTextSize(29f);
        String label = meter.getFps() < 0 ? "-- FPS" : meter.getFps() + " FPS";
        canvas.drawText(label, 52f, 43f, paint);

        GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, texture);
        if (textureAllocated) {
            GLUtils.texSubImage2D(GLES20.GL_TEXTURE_2D, 0, 0, 0, bitmap);
        } else {
            GLUtils.texImage2D(GLES20.GL_TEXTURE_2D, 0, bitmap, 0);
            textureAllocated = true;
        }
        dirty = false;
    }

    private static int bandColor(FrameRateMeter.Band band) {
        switch (band) {
            case RED: return Color.rgb(255, 64, 64);
            case AMBER: return Color.rgb(255, 192, 48);
            case GREEN: return Color.rgb(69, 225, 107);
            case BLUE: return Color.rgb(38, 175, 255);
            default: return Color.rgb(135, 142, 154);
        }
    }

    void shutdown() {
        if (texture != 0) {
            GLES20.glDeleteTextures(1, new int[] {texture}, 0);
            texture = 0;
        }
        textureAllocated = false;
        dirty = true;
        if (bitmap != null && !bitmap.isRecycled()) bitmap.recycle();
        bitmap = null;
        canvas = null;
        paint = null;
    }

    private static FloatBuffer allocate(float[] data) {
        FloatBuffer buffer = ByteBuffer.allocateDirect(data.length * 4)
            .order(ByteOrder.nativeOrder()).asFloatBuffer();
        buffer.put(data).position(0);
        return buffer;
    }
}
