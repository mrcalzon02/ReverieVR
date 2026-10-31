package org.beckenburg.editor;

import java.awt.BorderLayout;
import java.awt.Desktop;
import java.net.URI;
import javax.swing.JButton;
import javax.swing.JDialog;
import javax.swing.JFrame;
import javax.swing.JPanel;
import javax.swing.JScrollPane;
import javax.swing.JTabbedPane;
import javax.swing.JTextArea;

/** Lightweight offline reference, deliberately independent of Castle and of web access. */
public final class EditorAuthoringReference {
    private static final String WIKI =
        "https://github.com/mrcalzon02/The-Beckenburg-Project/blob/main/docs/wiki/psx-editor-engineering-guide.md";
    private static final String MACHINE =
        "https://raw.githubusercontent.com/mrcalzon02/The-Beckenburg-Project/main/docs/development/psx-editor-reference.json";
    private EditorAuthoringReference() {}

    private static JTextArea page(String body) {
        JTextArea view = new JTextArea(body, 20, 66);
        view.setEditable(false);
        view.setLineWrap(true);
        view.setWrapStyleWord(true);
        view.setCaretPosition(0);
        return view;
    }
    private static void browse(String url, JTextArea status) {
        try {
            if (!Desktop.isDesktopSupported() || !Desktop.getDesktop().isSupported(Desktop.Action.BROWSE))
                throw new IllegalStateException("No desktop browser is available");
            Desktop.getDesktop().browse(URI.create(url));
            status.setText("Opened " + url);
        } catch (Exception failure) {
            status.setText("Browser unavailable. Open this address manually:\n" + url);
        }
    }
    public static void show(JFrame parent) {
        JDialog dialog = new JDialog(parent, "PSX Editor - Authoring Reference", false);
        JTabbedPane tabs = new JTabbedPane();
        tabs.addTab("Scene", new JScrollPane(page(
            "RUNTIME FIRST / EDITOR SECOND\n\n" +
            "Use exactly the live game scene, renderer, assets, collision and simulation. " +
            "Keep stable object IDs, versioned saves, real layered transforms, Undo/Redo and safe playtest restoration.\n\n" +
            "Build geometry around genuine wall/floor/roof collision; basement stairs need a real opening. " +
            "Keep the same canonical van prefab in castle and investigations. " +
            "Use authoring gizmos, surface snapping and warnings, not blind edits.\n\n" +
            "Reference: Level Editor gold standards + PSX level building.")));
        tabs.addTab("Rendering / Light", new JScrollPane(page(
            "PSX LOOK / MODERN CORRECTNESS\n\n" +
            "Nearest-neighbor textures, limited palettes, quantized light, Gouraud gradients, affine warp, " +
            "vertex jitter and dithering are stylistic profiles, not actual PS1 hardware limits. " +
            "Never warp editor widgets, text or real collision coordinates.\n\n" +
            "The unified LightingModule, FixtureLighting and PsxPipeline own actual rendering. " +
            "Keep light reach separate from diffuse-edge width; respect occluding walls, doors and power circuits. " +
            "Use the real lighting reach overlay and profile door-opening invalidation. " +
            "Weather alone owns fog banks.\n\n" +
            "Reference: PSX lighting engine + PSX level building.")));
        tabs.addTab("AI / Nav", new JScrollPane(page(
            "AUTHOR PATHS / DO NOT CHEAT\n\n" +
            "Use valid portal and stair connections, real collision, door edge invalidation, " +
            "optional cached routes and tactical waypoint tags. Diagnose unreachable rooms.\n\n" +
            "The host remains authoritative; ghost hearing uses legitimate events and door/material/weather attenuation. " +
            "Neither player nor AI ghost may enter the van or hiding pockets, acquire omniscient player locations, " +
            "teleport through walls or bypass cooldowns. Measure CPU/GC before throttling optional decisions.\n\n" +
            "Reference: PSX NPC-AI-1 + Beckenburg AI adoption review.")));
        tabs.addTab("Asset Import", new JScrollPane(page(
            "MODEL INGESTION / OVER-REGRESSION\\n\\n" +
            "Use IMPORT MODEL inside the level editor to load OBJ, ASCII/binary STL or embedded-texture GLB. " +
            "The source is kept beside the exported GLB. GLB is validated through the real game decoder before it can be placed.\\n\\n" +
            "Geometry regression levels 0-12 use non-destructive vertex clustering. Inspect the SOURCE/RESULT comparison, " +
            "per-part triangle and corner counts, memory estimates and selectable PSX style budget. " +
            "An OVER-REGRESSION warning means part of the silhouette (for example a head or wheel) has collapsed. " +
            "Publishing a partially collapsed model needs explicit confirmation; a wholly empty mesh cannot be published.\\n\\n" +
            "GLB sources at level zero preserve their existing textures. Reduction of GLB currently loses material/mesh-node identity " +
            "and creates an explicit fallback atlas. Text glTF, FBX, DAE, PLY and Blender project parsing are not yet supported. " +
            "Material, bone, UV island and silhouette-preserving decimation are future upgrades. No Blender dependency in the game.\\n\\n" +
            "Reference: Developing assets for the PlayStation 1 (PSX) architecture, 13 pages; " +
            "docs/wiki/psx-asset-import-and-regression.md"
        )));
        tabs.addTab("Animation / SFX", new JScrollPane(page(
            "PSX CHARACTER ANIMATION + AUDIO AUTHORING\\n\\n" +
            "ANIMATE offers a portable 15/30 FPS named rigid-part timeline, stepped/linear preview, " +
            "separate root-motion keys, non-destructive redundant-key pruning, and atomically saved .anim files. " +
            "This is AUTHORING ONLY; it does not currently animate actual ghosts or change game/host authority.\\n\\n" +
            "SFX ASSETS inspects 8/16-bit PCM WAV and exports a separate optional mono 11.025/22.05kHz " +
            "8/16-bit derivative. Originals stay untouched. Sound import does not register a playback event " +
            "or install another mixer. Continue to use the game SoundVolume editor for real spatial acoustics.\\n\\n" +
            "Sources: PSX animations gold-1.pdf; PSX Arrow Sound Design.pdf; " +
            "docs/wiki/psx-animation-authoring.md and docs/wiki/psx-arrow-audio-design.md"
        )));
        tabs.addTab("Campaign / Billboard", new JScrollPane(page(
            "CAMPAIGN GRAPH / SOURCE-PRESERVING BILLBOARD IMAGES\\n\\n" +
            "CAMPAIGN provides an AUTHORING SIMULATION of Pending → Active ↔ Paused → Complete, " +
            "idempotent START/STOP actions, typed flag storage, reference dependencies and text save/load. " +
            "It is not connected to live mission, LAN or actor cleanup. Do not run untrusted scripts.\\n\\n" +
            "BILLBOARDS loads PNG/JPEG into a SOURCE versus 2D DERIVATIVE preview; selectable 256/512 " +
            "pixel canvas, optional palette reduction and ordered dithering, decoded-memory estimates, " +
            "and explicit PNG bake. The current preview is NOT yet a live SceneRenderer 3D billboard.\\n\\n" +
            "Sources: PSX-era level editor.pdf and owner-provided menu/campaign/billboard brief. " +
            "docs/wiki/psx-era-editor-campaign-tools.md; docs/wiki/psx-editor-menu-campaign-billboards.md"
        )));
        tabs.addTab("NPC / AI", new JScrollPane(page(
            "CANONICAL GHOST RULE READING / AI INSPECTION\\n\\n" +
            "The developer editor's NPC / AI button opens a read-only ghost rule lab. " +
            "It reads the REAL GhostIdentity, GhostTraits, GhostBehavior, GhostFrenzy and " +
            "GhostEvidenceMatrix definitions; no duplicate species table exists. " +
            "The isolated GhostDomain sandbox shows host-legal actions at a chosen 10Hz tick. " +
            "The PORTABLE editor.ai package provides typed rules, bounded legality reasoning, " +
            "victim-contact safeguards, deterministic replay and JSON rule export.\\n\\n" +
            "AUDIT ROSTER flags structural notes; EXPORT RULES JSON writes the current species " +
            "rules without active hunter data. It does not execute actions, change a live NPC, " +
            "create evidence, or override host-controlled hunting, hearing, navigation or multiplayer.\\n\\n" +
            "Reference: docs/wiki/psx-npc-ai-authoring.md; " +
            "docs/development/psx-npc-ai-authoring.json"
        )));
        tabs.addTab("UI / Tests", new JScrollPane(page(
            "AUTHENTIC PRESENTATION / NO ARTIFICIAL FRICTION\n\n" +
            "Compact menus, readable pixel styling and D-pad navigation are useful. Keep remapping, " +
            "mouse/keyboard/controller parity, accessible scale/contrast, readable text and instant input. " +
            "Never fake CD seek hangs, force memory-card mechanics or lock widescreen away.\n\n" +
            "Profile p50/p95/p99 frame time, draw work, postprocessing, light probes, heap/GC and AI. " +
            "Quality/aesthetic warnings remain diagnostic: only actual broken build, launch, required game play " +
            "or multiplayer may block release.\n\n" +
            "Reference: PSX ui guide and user + Level Editor gold standards.")));
        JPanel content = new JPanel(new BorderLayout(5, 5));
        content.add(tabs, BorderLayout.CENTER);
        JPanel footer = new JPanel(new BorderLayout(4, 4));
        JPanel buttons = new JPanel();
        JTextArea status = page("Offline summary is bundled in the standalone editor kit. Full sources: project wiki / machine JSON.");
        status.setRows(2);
        JButton wiki = new JButton("Human guide");
        wiki.addActionListener(e -> browse(WIKI, status));
        JButton data = new JButton("Machine JSON");
        data.addActionListener(e -> browse(MACHINE, status));
        JButton close = new JButton("Close");
        close.addActionListener(e -> dialog.dispose());
        buttons.add(wiki); buttons.add(data); buttons.add(close);
        footer.add(buttons, BorderLayout.NORTH);
        footer.add(status, BorderLayout.CENTER);
        content.add(footer, BorderLayout.SOUTH);
        dialog.setContentPane(content);
        dialog.setSize(660, 510);
        dialog.setLocationRelativeTo(parent);
        dialog.setVisible(true);
    }
}
