package dev.mrcalzon.water;
import javax.swing.*;
import java.awt.*;
import java.awt.event.*;
import java.util.*;
import java.util.List;
import java.util.function.Supplier;

/** Optional portable Swing inspector. Placement, transforms, multi-edit,
 * texture sheets and flow controls work without any Beckenburg imports. */
public final class WaterEditorPanel extends JPanel {
 public record Origin(double x,double y,double z){}
 private java.util.function.Function<String,String> texturePicker=key->null;private final java.util.List<JButton> browseButtons=new ArrayList<>();private WaterEditorModel model;private Runnable unlisten=()->{};private final Supplier<Origin> origin;private final Runnable save;
 private final DefaultListModel<WaterPlacement> rows=new DefaultListModel<>();private final JList<WaterPlacement> list=new JList<>(rows);
 private final Map<String,JSpinner> numbers=new LinkedHashMap<>();private final Map<String,JTextField> text=new LinkedHashMap<>();private final Map<String,JCheckBox> toggles=new LinkedHashMap<>();private final JCheckBox snapEdges=new JCheckBox("Edge snap",true),snapAngles=new JCheckBox("Angle snap",true);private final JComboBox<WaterPlacement.Shape> shape=new JComboBox<>(WaterPlacement.Shape.values());private final JLabel status=new JLabel("Place a water module");private boolean refreshing;
 public WaterEditorPanel(WaterEditorModel model,Supplier<Origin> origin,Runnable save){
  super(new BorderLayout(4,4));this.origin=Objects.requireNonNull(origin);this.save=Objects.requireNonNull(save);
  var toolbar=new JPanel(new GridLayout(0,3,3,3));var add=new JButton("Water plane",new WaterIcon());add.setName("water.add");add.setToolTipText("Place a complete water surface ahead; resize and configure it below");add.addActionListener(e->{var p=this.origin.get();place(p.x,p.y,p.z,8,8);});toolbar.add(add);
  action(toolbar,"Copy",()->this.model.duplicateSelected(2,2));action(toolbar,"Delete",()->this.model.deleteSelected());action(toolbar,"Undo",()->this.model.undo());action(toolbar,"Redo",()->this.model.redo());action(toolbar,"Save",()->this.save.run());toolbar.add(snapEdges);toolbar.add(snapAngles);action(toolbar,"Snap selected",()->this.model.snapSelected(.4,snapAngles.isSelected()));add(toolbar,BorderLayout.NORTH);
  list.setSelectionMode(ListSelectionModel.MULTIPLE_INTERVAL_SELECTION);list.setVisibleRowCount(4);list.setCellRenderer((owner,p,index,selected,focus)->{var label=new JLabel(p.id()+"  "+String.format(Locale.ROOT,"%.1f×%.1fm / Y%.2f",p.width(),p.depth(),p.y()),new WaterIcon(),JLabel.LEFT);label.setOpaque(true);label.setBackground(selected?new Color(45,83,105):owner.getBackground());label.setForeground(selected?Color.WHITE:owner.getForeground());return label;});
  list.addListSelectionListener(e->{if(!refreshing&&!e.getValueIsAdjusting())this.model.select(list.getSelectedValuesList().stream().map(WaterPlacement::id).toList());});
  var fields=new JPanel(new GridLayout(0,2,4,3));shape.setName("water.shape");shape.getAccessibleContext().setAccessibleName("Water footprint shape");fields.add(new JLabel("Footprint shape"));fields.add(shape);shape.addActionListener(e->{if(!refreshing)edit("$shape",shape.getSelectedItem());});
  numeric(fields,"$x","Position X",-10000,10000,.1,0);numeric(fields,"$y","Water surface Y",-10000,10000,.05,0);numeric(fields,"$z","Position Z",-10000,10000,.1,0);numeric(fields,"$width","Width (m)",.1,2000,.5,8);numeric(fields,"$depth","Length (m)",.1,2000,.5,8);numeric(fields,"$yaw","Rotation degrees",-36000,36000,5,0);
  for(var spec:WaterSettings.NUMBERS)numeric(fields,spec.key(),spec.label(),spec.minimum(),spec.maximum(),spec.step(),spec.initial());
  for(String key:List.of("layer1.texture","layer2.texture","colour.shallow","colour.deep","colour.reflection","bed.profile")){var input=new JTextField();input.setName("water."+key);input.getAccessibleContext().setAccessibleName(key);text.put(key,input);fields.add(new JLabel(key));if(key.endsWith(".texture")){var row=new JPanel(new BorderLayout(2,0));row.add(input,BorderLayout.CENTER);var browse=new JButton("…");browse.setToolTipText("Choose a PNG texture sheet");browseButtons.add(browse);browse.addActionListener(e->run(()->{String uri=texturePicker.apply(key);if(uri!=null)model.edit(key,uri);}));row.add(browse,BorderLayout.EAST);fields.add(row);}else fields.add(input);input.addActionListener(e->edit(key,input.getText().strip()));input.setToolTipText("Press Enter to apply to the selected modules");}
  for(String key:List.of("contact.enabled","spray.enabled")){var input=new JCheckBox(key);input.setName("water."+key);toggles.put(key,input);fields.add(input);fields.add(new JLabel(""));input.addActionListener(e->{if(!refreshing)edit(key,input.isSelected());});}
  var center=new JPanel(new BorderLayout(4,4));center.add(new JScrollPane(list),BorderLayout.NORTH);center.add(new JScrollPane(fields),BorderLayout.CENTER);add(center,BorderLayout.CENTER);
  var footer=new JPanel(new BorderLayout(3,3));var match=new JButton("Match selected settings (seamless) ");match.addActionListener(e->run(()->this.model.matchSettings()));footer.add(match,BorderLayout.NORTH);footer.add(status,BorderLayout.SOUTH);add(footer,BorderLayout.SOUTH);
  bind("control Z","undo",()->this.model.undo());bind("control Y","redo",()->this.model.redo());bind("control S","save",()->this.save.run());setModel(model);
 }
 private void bind(String key,String id,Runnable run){getInputMap(WHEN_ANCESTOR_OF_FOCUSED_COMPONENT).put(KeyStroke.getKeyStroke(key),id);getActionMap().put(id,new AbstractAction(){public void actionPerformed(ActionEvent e){run(run);}});}
 private void action(JPanel panel,String name,Runnable action){var b=new JButton(name);b.addActionListener(e->run(action));panel.add(b);}
 private void numeric(JPanel fields,String key,String label,double minimum,double maximum,double step,double initial){var input=new JSpinner(new SpinnerNumberModel(initial,minimum,maximum,step));input.setName("water."+key);input.getAccessibleContext().setAccessibleName(label);numbers.put(key,input);fields.add(new JLabel(label));fields.add(input);input.addChangeListener(e->{if(!refreshing)edit(key,input.getValue());});}
 public void setModel(WaterEditorModel model){unlisten.run();this.model=Objects.requireNonNull(model);unlisten=model.listen(this::refresh);refresh();}
 public void texturePicker(java.util.function.Function<String,String> picker){texturePicker=java.util.Objects.requireNonNull(picker);}
 public boolean snappingEnabled(){return snapEdges.isSelected();}
 public boolean angleSnappingEnabled(){return snapAngles.isSelected();}
 public WaterEditorModel model(){return model;}
 public void place(double x,double y,double z,double width,double depth){run(()->model.add(x,y,z,width,depth));}
 public void select(Collection<String> ids){model.select(ids);}
 private void edit(String key,Object value){run(()->model.edit(key,value));}
 private void run(Runnable action){try{action.run();status.setText((model.dirty()?"Unsaved · ":"")+model.selected().size()+" selected · "+model.document().placements().size()+" water modules");}catch(RuntimeException error){refresh();status.setText("Water: "+error.getMessage());}}
 private void refresh(){refreshing=true;try{rows.clear();for(var p:model.document().placements())rows.addElement(p);var selection=model.selected();list.setSelectedIndices(java.util.stream.IntStream.range(0,rows.size()).filter(i->selection.contains(rows.get(i))).toArray());boolean enabled=!selection.isEmpty();var p=enabled?selection.get(0):null;var settings=p==null?new WaterSettings():p.settings();shape.setSelectedItem(p==null?WaterPlacement.Shape.SQUARE:p.shape());shape.setEnabled(enabled);for(var button:browseButtons)button.setEnabled(enabled);
  for(var entry:numbers.entrySet()){String key=entry.getKey();double v=key.startsWith("$")?(p==null?0:switch(key){case "$x"->p.x();case "$y"->p.y();case "$z"->p.z();case "$width"->p.width();case "$depth"->p.depth();default->Math.toDegrees(p.yaw());}):settings.number(key);if(p==null&&(key.equals("$width")||key.equals("$depth")))v=8;entry.getValue().setValue(v);entry.getValue().setEnabled(enabled);}
  for(var entry:text.entrySet()){entry.getValue().setText(settings.text(entry.getKey()));entry.getValue().setEnabled(enabled);}for(var entry:toggles.entrySet()){entry.getValue().setSelected(settings.enabled(entry.getKey()));entry.getValue().setEnabled(enabled);}status.setText((model.dirty()?"Unsaved · ":"")+selection.size()+" selected · "+rows.size()+" water modules");
 }finally{refreshing=false;}}
}
