package org.beckenburg.editor;

import javax.swing.*;
import java.awt.*;
import java.util.List;
import java.util.function.Consumer;

/** Reusable compact asset-list dock. No project-specific class is referenced. */
public final class EditorPalettePanel extends JPanel {
    private final JComboBox<String> category=new JComboBox<>();
    private final JTextField filter=new JTextField(12);
    private final DefaultListModel<EditorProject.Asset> items=new DefaultListModel<>();
    private final JList<EditorProject.Asset> list=new JList<>(items);
    private List<EditorProject.Asset> assets;
    public EditorPalettePanel(EditorProject<?> project,Consumer<EditorProject.Asset> select){
        super(new BorderLayout(3,3));
        assets=List.copyOf(project.assets());
        category.addItem("All");
        assets.stream().map(EditorProject.Asset::category).distinct().sorted()
            .forEach(category::addItem);
        JPanel controls=new JPanel(new BorderLayout(3,3));
        controls.add(category,BorderLayout.WEST);controls.add(filter,BorderLayout.CENTER);
        add(controls,BorderLayout.NORTH);
        list.setCellRenderer((JList<? extends EditorProject.Asset> owner,
                              EditorProject.Asset value,int index,boolean selected,boolean focus)->{
            JLabel label=new JLabel(value.label());
            label.setOpaque(true);label.setBackground(selected?new Color(66,91,119):owner.getBackground());
            label.setForeground(selected?Color.WHITE:owner.getForeground());
            return label;
        });
        add(new JScrollPane(list),BorderLayout.CENTER);
        list.addListSelectionListener(e->{if(!e.getValueIsAdjusting()&&list.getSelectedValue()!=null)
            select.accept(list.getSelectedValue());});
        category.addActionListener(e->refresh());
        filter.getDocument().addDocumentListener(new javax.swing.event.DocumentListener(){
            public void insertUpdate(javax.swing.event.DocumentEvent e){refresh();}
            public void removeUpdate(javax.swing.event.DocumentEvent e){refresh();}
            public void changedUpdate(javax.swing.event.DocumentEvent e){refresh();}
        });
        refresh();
    }
    /** Refresh the actual game/project catalog after an import, preserving filtering. */
    public void reload(EditorProject<?> project){
        assets=List.copyOf(project.assets());
        String previous=(String)category.getSelectedItem();
        category.removeAllItems();
        category.addItem("All");
        assets.stream().map(EditorProject.Asset::category).distinct().sorted()
            .forEach(category::addItem);
        category.setSelectedItem(previous!=null&&
            assets.stream().anyMatch(asset->asset.category().equals(previous))?previous:"All");
        refresh();
    }

    private void refresh(){
        String q=filter.getText().strip().toLowerCase(java.util.Locale.ROOT);
        String group=(String)category.getSelectedItem();
        items.clear();
        for(var asset:assets)if(("All".equals(group)||asset.category().equals(group))
            &&(q.isEmpty()||asset.label().toLowerCase(java.util.Locale.ROOT).contains(q)))
            items.addElement(asset);
    }
}
