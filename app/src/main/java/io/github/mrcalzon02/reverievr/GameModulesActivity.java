package io.github.mrcalzon02.reverievr;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.TextView;

import java.util.List;

/**
 * Read-only inventory of native games shipped inside the ReverieVR APK.
 * Game code and game assets update through the signed whole-app updater.
 * No per-game network fetch, package installer, or local rvpack import.
 */
public final class GameModulesActivity extends Activity {
    @Override public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        ScrollView scroll = new ScrollView(this);
        LinearLayout content = new LinearLayout(this);
        content.setOrientation(LinearLayout.VERTICAL);
        content.setPadding(20, 20, 20, 20);
        scroll.addView(content);

        TextView explanation = new TextView(this);
        explanation.setText("Bundled Game Library\nAll native games, their code, and their approved assets are delivered with ReverieVR. Update the main app once to update the entire library. Games launch from inside VR Home.");
        explanation.setTextSize(17);
        content.addView(explanation);

        Button update = new Button(this);
        update.setText("Check ReverieVR updates");
        update.setOnClickListener(view -> {
            Intent intent = new Intent(this, MainActivity.class);
            intent.putExtra(MainActivity.EXTRA_CHECK_BUNDLED_UPDATE, true);
            intent.addFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP
                | Intent.FLAG_ACTIVITY_SINGLE_TOP);
            startActivity(intent);
            finish();
        });
        content.addView(update);

        boolean development = new ReveriePreferences(this)
            .getLoggingMode() == LoggingMode.DEVELOPMENT;
        List<NativeModuleRuntime.Descriptor> games =
            NativeModuleRuntime.listBuiltIns(true);
        TextView summary = new TextView(this);
        summary.setText(games.isEmpty()
            ? "Bundled native games could not be enumerated. Check the installed APK."
            : games.size() + " native games included in this APK.");
        content.addView(summary);

        for (NativeModuleRuntime.Descriptor game : games) {
            GameModuleRegistry.Entry info = GameModuleRegistry.find(game.id);
            TextView item = new TextView(this);
            String label = game.displayName + " — Included";
            if (info != null && info.developmentOnly && !development)
                label += " (enable Development logging mode to launch)";
            item.setText(label);
            item.setPadding(0, 14, 0, 14);
            content.addView(item);
        }
        setContentView(scroll);
    }
}
