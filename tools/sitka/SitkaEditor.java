import dev.mrcalzon.water.WaterCanvas;
import dev.mrcalzon.water.WaterDocument;
import dev.mrcalzon.water.WaterEditorModel;
import dev.mrcalzon.water.WaterEditorPanel;
import dev.mrcalzon.water.WaterPlacement;
import dev.mrcalzon.weather.WeatherModule;
import dev.mrcalzon.weather.WeatherType;
import dev.mrcalzon.weather.WeatherView;
import org.beckenburg.editor.EditorPalettePanel;
import org.beckenburg.editor.EditorProject;

import javax.swing.*;
import java.awt.*;
import java.awt.event.WindowAdapter;
import java.awt.event.WindowEvent;
import java.io.IOException;
import java.io.InputStream;
import java.nio.file.*;
import java.util.List;
import java.util.Properties;

/** Separate Sitka authoring host using the genuine Beckenburg Java17 editor,
 * water-module and shared PSX weather code used in Beckenburg/Malefic Veil.
 * This is a DESKTOP editor; the future Daydream runtime gets baked scene data.
 */
public final class SitkaEditor {
    private final Path root;
    private final Path waterFile, weatherFile;
    private final WaterEditorModel water;
    private final WeatherModule weather = new WeatherModule(0x517CA1L);
    private final JFrame frame = new JFrame("Sitka - Coastal Environment Editor");
    private final JLabel status = new JLabel();
    private final JComboBox<WeatherType> mode = new JComboBox<>(new WeatherType[]{
        WeatherType.CLEAR, WeatherType.OVERCAST, WeatherType.DRIZZLE,
        WeatherType.RAIN, WeatherType.HEAVY_RAIN, WeatherType.FOG,
        WeatherType.DENSE_FOG, WeatherType.SNOW, WeatherType.THUNDERSTORM});
    private final JSpinner hour = new JSpinner(new SpinnerNumberModel(13.0, 0.0, 23.9, 0.1));
    private final JSpinner wind = new JSpinner(new SpinnerNumberModel(1.0, 0.0, 60.0, 0.5));
    private final JSpinner heading = new JSpinner(new SpinnerNumberModel(0.0, 0.0, 360.0, 5.0));
    private final Timer timer;
    private boolean weatherDirty = false;

    private SitkaEditor(Path root) throws IOException {
        this.root = root.toAbsolutePath().normalize();
        waterFile = this.root.resolve("water/sitka.water");
        weatherFile = this.root.resolve("weather/sitka.properties");
        WaterDocument original;
        if (Files.isRegularFile(waterFile)) {
            try (InputStream stream = Files.newInputStream(waterFile)) {
                original = WaterDocument.read(stream);
            }
            if (!"sitka".equals(original.level())) throw new IOException("Not a Sitka water document");
        } else {
            original = new WaterDocument("sitka", List.of());
        }
        water = new WaterEditorModel(original);
        loadWeather();
        weather.setTimeOfDay(number(hour)/24.0);
        weather.setWeather((WeatherType)mode.getSelectedItem(), 0);
        weather.weather().setWind(number(wind), Math.toRadians(number(heading)));

        WaterEditorPanel inspector = new WaterEditorPanel(water,
            () -> new WaterEditorPanel.Origin(0, 0, 0), this::saveWithDialog);
        WaterCanvas canvas = new WaterCanvas(water, inspector::snappingEnabled,
            inspector::angleSnappingEnabled, () -> 0.0);
        EditorProject<WaterDocument> project = new EditorProject<>() {
            @Override public String id() { return "sitka"; }
            @Override public String title() { return "Sitka"; }
            @Override public Path documentPath() { return waterFile; }
            @Override public List<Node> hierarchy() {
                return water.document().placements().stream()
                    .map(p -> new Node(p.id(), "water", p.id(), p.x(), p.y(), p.z())).toList();
            }
            @Override public List<Asset> assets() {
                return List.of(new Asset("sitka://water/surface", "Water", "Place water surface"));
            }
            @Override public WaterDocument snapshot() { return water.document(); }
            @Override public void restore(WaterDocument saved) {
                // Palette adapter does not own independent history: the water model
                // already implements bounded transactional undo/redo.
                throw new UnsupportedOperationException("Use water editor undo/redo");
            }
            @Override public void save() throws IOException { SitkaEditor.this.save(); }
        };
        EditorPalettePanel palette = new EditorPalettePanel(project, item -> {
            if ("sitka://water/surface".equals(item.uri())) {
                try { water.add(0, 0, 0, 8, 8); canvas.fit(); }
                catch (IllegalArgumentException failure) { status.setText(failure.getMessage()); }
            }
        });
        var weatherPanel = buildWeatherPanel();
        JTabbedPane controls = new JTabbedPane();
        controls.addTab("Water inspector", inspector);
        controls.addTab("Weather", weatherPanel);
        controls.addTab("PSX palette", palette);
        JSplitPane split = new JSplitPane(JSplitPane.HORIZONTAL_SPLIT, canvas, controls);
        split.setResizeWeight(.68);
        JPanel full = new JPanel(new BorderLayout(4, 4));
        JPanel toolbar = new JPanel(new FlowLayout(FlowLayout.LEFT));
        JButton save = new JButton("Save water + weather");
        save.addActionListener(e -> saveWithDialog());
        toolbar.add(save);
        toolbar.add(new JLabel("Authoring tools: Beckenburg water/editor + shared weather"));
        full.add(toolbar, BorderLayout.NORTH);
        full.add(split, BorderLayout.CENTER);
        full.add(status, BorderLayout.SOUTH);
        frame.setContentPane(full);
        frame.setSize(1360, 840);
        frame.setDefaultCloseOperation(WindowConstants.DO_NOTHING_ON_CLOSE);
        frame.addWindowListener(new WindowAdapter() {
            @Override public void windowClosing(WindowEvent e) { close(); }
        });
        status.setText("Sitka workspace: " + this.root);
        // Preview only: no runtime gameplay or real audio output is implied.
        timer = new Timer(50, e -> {
            weather.advance(.05, WeatherView.outdoors(0, 1.7, 0, 0));
            weatherPanel.repaint();
        });
    }

    private JPanel buildWeatherPanel() {
        JPanel p = new JPanel(new BorderLayout(4,4));
        JPanel fields = new JPanel(new GridLayout(0, 2, 4, 5));
        fields.add(new JLabel("Condition")); fields.add(mode);
        fields.add(new JLabel("Hour")); fields.add(hour);
        fields.add(new JLabel("Wind m/s")); fields.add(wind);
        fields.add(new JLabel("Wind heading degrees")); fields.add(heading);
        p.add(fields, BorderLayout.NORTH);
        JPanel sky = new JPanel() {
            @Override protected void paintComponent(Graphics graphics) {
                super.paintComponent(graphics);
                weather.renderSky((Graphics2D)graphics, getWidth(), getHeight());
                weather.renderOverlay((Graphics2D)graphics, getWidth(), getHeight());
            }
        };
        p.add(sky, BorderLayout.CENTER);
        mode.addActionListener(e -> {
            if (mode.getSelectedItem() != null) {
                weather.setWeather((WeatherType)mode.getSelectedItem(), 5);
                weatherDirty = true;
            }
        });
        hour.addChangeListener(e -> { weather.setTimeOfDay(number(hour)/24); weatherDirty=true; });
        wind.addChangeListener(e -> updateWind());
        heading.addChangeListener(e -> updateWind());
        return p;
    }
    private static double number(JSpinner spinner) { return ((Number)spinner.getValue()).doubleValue(); }
    private void updateWind() {
        weather.weather().setWind(number(wind), Math.toRadians(number(heading)));
        weatherDirty = true;
    }
    private void loadWeather() throws IOException {
        if (!Files.exists(weatherFile)) return;
        Properties p = new Properties();
        try (var in = Files.newInputStream(weatherFile)) { p.load(in); }
        if (!"1".equals(p.getProperty("version"))) throw new IOException("Unsupported Sitka weather version");
        try {
            mode.setSelectedItem(WeatherType.valueOf(p.getProperty("condition")));
            hour.setValue(Double.parseDouble(p.getProperty("hour")));
            wind.setValue(Double.parseDouble(p.getProperty("wind_mps")));
            heading.setValue(Double.parseDouble(p.getProperty("wind_heading_deg")));
        } catch (RuntimeException error) { throw new IOException("Invalid saved Sitka weather", error); }
        weatherDirty = false;
    }
    private static void atomicSave(Path target, String text) throws IOException {
        Files.createDirectories(target.getParent());
        Path tmp = Files.createTempFile(target.getParent(), ".sitka-", ".tmp");
        try {
            Files.writeString(tmp, text);
            try { Files.move(tmp,target,StandardCopyOption.REPLACE_EXISTING,StandardCopyOption.ATOMIC_MOVE); }
            catch (AtomicMoveNotSupportedException e) {
                Files.move(tmp,target,StandardCopyOption.REPLACE_EXISTING);
            }
        } finally { Files.deleteIfExists(tmp); }
    }
    private void save() throws IOException {
        // Commit both authored documents. Weather source is not a fake on-phone
        // weather runtime: it is the profile the future native adapter will parse.
        String props = "version=1\ncondition=" + mode.getSelectedItem() +
            "\nhour=" + number(hour) + "\nwind_mps=" + number(wind) +
            "\nwind_heading_deg=" + number(heading) + "\n";
        water.document().save(waterFile);
        atomicSave(weatherFile, props);
        water.markSaved();
        weatherDirty = false;
        status.setText("Saved authentic water document and weather profile: " + root);
    }
    private void saveWithDialog() {
        try { save(); } catch (IOException ex) {
            status.setText("Save failed: " + ex.getMessage());
            JOptionPane.showMessageDialog(frame, ex.getMessage(), "Save failed", JOptionPane.ERROR_MESSAGE);
        }
    }
    private void close() {
        if (water.dirty() || weatherDirty) {
            int value = JOptionPane.showConfirmDialog(frame,"Save Sitka water and weather edits?",
                "Unsaved environment",JOptionPane.YES_NO_CANCEL_OPTION);
            if (value == JOptionPane.CANCEL_OPTION || value == JOptionPane.CLOSED_OPTION) return;
            if (value == JOptionPane.YES_OPTION) {
                try { save(); } catch (IOException e) { status.setText(e.getMessage()); return; }
            }
        }
        timer.stop();
        frame.dispose();
    }
    public static void main(String[] args) throws Exception {
        if (args.length != 1) {
            System.err.println("Usage: SitkaEditor <workspace-directory>");
            System.exit(2);
        }
        if (GraphicsEnvironment.isHeadless()) throw new IllegalStateException("Desktop editor requires a display");
        SitkaEditor editor = new SitkaEditor(Path.of(args[0]));
        SwingUtilities.invokeLater(() -> { editor.frame.setLocationRelativeTo(null); editor.frame.setVisible(true); editor.timer.start(); });
    }
}
