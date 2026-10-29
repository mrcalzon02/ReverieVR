package io.github.mrcalzon02.reverievr;

import java.awt.Color;
import java.awt.Font;
import java.awt.Graphics2D;
import java.awt.geom.Path2D;
import java.awt.geom.Point2D;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

/** The visible Home desk keys and their matching perspective mouse targets. */
final class DesktopDeskKeyboard {
    private static final double X0 = -1.25, Z0 = -2.41, Y = -.265;
    private static final double DX = .18, DZ = .19, WIDTH = .15, DEPTH = .14;
    private static final String[][] LABELS = {
        {"1","2","3","4","5","6","7","8","9","0","-","/","BKSP","ENTER"},
        {"Q","W","E","R","T","Y","U","I","O","P","A","S","D","F"},
        {"G","H","J","K","L","Z","X","C","V","B","N","M","SPACE","SEARCH"}
    };

    static final class Key {
        final int row, col;
        final String label;
        private Key(int row, int col, String label) {
            this.row = row;
            this.col = col;
            this.label = label;
        }
    }

    static final List<Key> KEYS;
    static {
        List<Key> keys = new ArrayList<>();
        for (int row = 0; row < LABELS.length; row++)
            for (int col = 0; col < LABELS[row].length; col++)
                keys.add(new Key(row, col, LABELS[row][col]));
        KEYS = Collections.unmodifiableList(keys);
    }

    static Key key(String label) {
        for (Key key : KEYS) if (key.label.equals(label)) return key;
        return null;
    }

    private static Point2D project(DesktopHubState state, int width, int height,
                                   double x, double y, double z) {
        double dx = x - state.x, dz = z - state.z;
        double sideways = Math.cos(state.yaw) * dx + Math.sin(state.yaw) * dz;
        double distance = Math.sin(state.yaw) * dx - Math.cos(state.yaw) * dz;
        if (distance < .15 || width <= 0 || height <= 0) return null;
        double focal = Math.min(width, height) * 1.08;
        return new Point2D.Double(width * .5 + sideways * focal / distance,
            height * .52 - (y - 1.0) * focal / distance);
    }

    static Point2D[] corners(DesktopHubState state, int width, int height, Key key) {
        if (key == null) return null;
        double x = X0 + key.col * DX, z = Z0 + key.row * DZ;
        Point2D[] points = {
            project(state, width, height, x, Y, z),
            project(state, width, height, x + WIDTH, Y, z),
            project(state, width, height, x + WIDTH, Y, z + DEPTH),
            project(state, width, height, x, Y, z + DEPTH)
        };
        for (Point2D point : points) if (point == null) return null;
        return points;
    }

    private static Path2D outline(Point2D[] points) {
        Path2D.Double shape = new Path2D.Double();
        shape.moveTo(points[0].getX(), points[0].getY());
        for (int i = 1; i < points.length; i++)
            shape.lineTo(points[i].getX(), points[i].getY());
        shape.closePath();
        return shape;
    }

    static Key hit(DesktopHubState state, int width, int height, double x, double y) {
        if (x < 0 || y < 0 || x >= width || y >= height) return null;
        for (Key key : KEYS) {
            Point2D[] points = corners(state, width, height, key);
            if (points != null && outline(points).contains(x, y)) return key;
        }
        return null;
    }

    /** Returns search-focus state; only the actual Games panel accepts text. */
    static boolean press(DesktopHubState state, boolean focused, Key key) {
        if (key == null || !state.runningGame.isEmpty()) return focused;
        if (key.label.equals("SEARCH")) {
            state.open(DesktopHubState.Panel.GAMES);
            return true;
        }
        if (state.panel != DesktopHubState.Panel.GAMES
            && state.panel != DesktopHubState.Panel.NONE) return focused;
        if (key.label.equals("ENTER"))
            return state.panel == DesktopHubState.Panel.GAMES ? false : focused;
        if (state.panel == DesktopHubState.Panel.NONE)
            state.open(DesktopHubState.Panel.GAMES);
        if (key.label.equals("BKSP")) state.search('\b');
        else if (key.label.equals("SPACE")) state.search(' ');
        else if (key.label.length() == 1)
            state.search(Character.toLowerCase(key.label.charAt(0)));
        return true;
    }

    static void draw(Graphics2D g, DesktopHubState state, int width, int height,
                     boolean searchFocused) {
        for (Key key : KEYS) {
            Point2D[] points = corners(state, width, height, key);
            if (points == null) continue;
            Path2D shape = outline(points);
            boolean action = key.label.length() > 1;
            g.setColor(action ? new Color(67, 115, 142) :
                (key.row + key.col) % 2 == 0 ? new Color(111, 134, 150) : new Color(77, 100, 118));
            g.fill(shape);
            g.setColor(searchFocused && key.label.equals("SEARCH") ?
                new Color(247, 201, 105) : new Color(25, 43, 57));
            g.draw(shape);
            double centerX = 0, centerY = 0;
            for (Point2D point : points) {
                centerX += point.getX() / points.length;
                centerY += point.getY() / points.length;
            }
            int available = (int) Math.max(8, points[1].getX() - points[0].getX() - 3);
            int fontSize = Math.max(8, Math.min(14, available / Math.max(1, key.label.length())));
            Font font = new Font(Font.MONOSPACED, Font.BOLD, fontSize);
            g.setFont(font);
            while (fontSize > 7 && g.getFontMetrics().stringWidth(key.label) > available) {
                font = font.deriveFont((float) --fontSize);
                g.setFont(font);
            }
            g.setColor(new Color(242, 246, 246));
            g.drawString(key.label,
                (float) (centerX - g.getFontMetrics().stringWidth(key.label) * .5),
                (float) (centerY + g.getFontMetrics().getAscent() * .35));
        }
    }

    private DesktopDeskKeyboard() { }
}
