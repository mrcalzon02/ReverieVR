"""Run: blender -b -t 4 --python build_helm.py. Asset study, no game wiring."""
import bpy, math, json
from mathutils import Vector
from pathlib import Path
P=Path(__file__).resolve().parent
bpy.ops.object.select_all(action='SELECT'); bpy.ops.object.delete(use_global=False)
def mat(name,color):
    m=bpy.data.materials.new(name);m.diffuse_color=(*color,1);return m
gray=mat('flat gray plate',(.29,.33,.34));black=mat('charcoal bezels',(.035,.045,.05));metal=mat('simple steel',(.46,.49,.47));gold=mat('muted brass',(.65,.46,.12));red=mat('red switch indicators',(.65,.07,.025))
root=bpy.data.objects.new('MAXIMUM_HELM_ROOT',None);bpy.context.collection.objects.link(root)
root.rotation_euler.x=math.radians(68);root.location.z=.85
def finish(o,name,material):
    o.name=name;o.parent=root;o.data.materials.append(material);o['asset_role']='removable physical console component';return o
def box(name,loc,size,material):
    bpy.ops.mesh.primitive_cube_add(size=1,location=loc);o=bpy.context.object;o.scale=size
    bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
    return finish(o,name,material)
def cyl(name,loc,r,depth,material,n=16):
    bpy.ops.mesh.primitive_cylinder_add(vertices=n,radius=r,depth=depth,location=loc);return finish(bpy.context.object,name,material)
def bar(name,a,b,r,material):
    a,b=Vector(a),Vector(b);o=cyl(name,(a+b)/2,r,(b-a).length,material,8);o.rotation_euler=(b-a).to_track_quat('Z','Y').to_euler();return o
def image_mat(name,path,sequence=False):
    m=bpy.data.materials.new(name);m.use_nodes=True;n=m.node_tree.nodes;n.clear()
    out=n.new('ShaderNodeOutputMaterial');em=n.new('ShaderNodeEmission');tex=n.new('ShaderNodeTexImage')
    tex.image=bpy.data.images.load(str(path));tex.interpolation='Closest'
    if sequence:
        tex.image.source='SEQUENCE';tex.image_user.frame_duration=24;tex.image_user.use_auto_refresh=True;tex.image_user.use_cyclic=True
    m.node_tree.links.new(tex.outputs['Color'],em.inputs[0]);m.node_tree.links.new(em.outputs[0],out.inputs[0]);return m
def face(name,x,y,z,w,h,material):
    mesh=bpy.data.meshes.new(name);mesh.from_pydata([(x-w/2,y-h/2,z),(x+w/2,y-h/2,z),(x+w/2,y+h/2,z),(x-w/2,y+h/2,z)],[],[(0,1,2,3)])
    mesh.uv_layers.new()
    for loop,uv in zip(mesh.uv_layers[0].data,[(0,0),(1,0),(1,1),(0,1)]):loop.uv=uv
    o=bpy.data.objects.new(name,mesh);bpy.context.collection.objects.link(o);return finish(o,name,material)
box('console display plate',(0,0,0),(1.72,1.04,.095),gray)
box('console lower housing',(0,-.04,-.16),(1.66,.94,.26),gray)
box('lower brass edge',(0,-.525,-.025),(1.70,.018,.045),gold)
for x in (-.79,.79):
    for y in (-.46,.46):cyl('plate mounting bolt',(x,y,.057),.012,.014,gold,6)
for x in (-.78,.78):bar('protective rail upright',(x,.48,-.04),(x,.64,.04),.022,metal)
bar('protective rail crossbar',(-.78,.64,.04),(.78,.64,.04),.022,metal)
for x in (-.47,.47):
    box('work light bracket',(x,.61,.08),(.07,.06,.07),metal)
    box('work light cube',(x,.66,.13),(.23,.13,.09),black)
    face('work light face',x,.66,.181,.19,.085,gold)
modes={k:image_mat(k,P/'textures'/f'{k.lower()}.png') for k in ['CHART','SONAR','RADAR','CAMERA']}
for index,x in enumerate((-.46,.08)):
    box(f'display {index} removable mount',(x,.23,.085),(.47,.39,.07),black)
    f=face(f'display {index} mode surface',x-.028,.23,.126,.35,.29,modes['CHART' if index==0 else 'SONAR'])
    # Material changes make the saved Blender timeline demonstrate mode selection.
    for m in modes.values():
        if m not in list(f.data.materials):f.data.materials.append(m)
    for frame,mode in [(1,'CHART' if index==0 else 'SONAR'),(7,'RADAR' if index==0 else 'CAMERA'),(13,'CHART' if index==0 else 'SONAR'),(19,'RADAR' if index==0 else 'CAMERA')]:
        f.data.polygons[0].material_index=list(f.data.materials).index(modes[mode]);f.data.polygons[0].keyframe_insert('material_index',frame=frame)
    for j,mode in enumerate(['CHART','SONAR','RADAR','CAMERA']):
        b=box(f'display {index} {mode} button',(x+.19,.33-j*.066,.135),(.037,.040,.020),metal);b['proposed_action']='select_'+mode.lower();b['runtime_wired']=False
    cyl(f'display {index} rotary knob',(x+.19,.088,.14),.025,.025,metal,12)
box('VHF radio module',(.59,.27,.10),(.34,.28,.10),black)
face('VHF status face',.55,.31,.156,.19,.08,image_mat('VHF demo status',P/'textures/vhf.png'))
for j in range(3):
    for i in range(4):box('VHF keypad',(.47+i*.044,.245-j*.033,.161),(.027,.020,.012),metal)
box('radio handset',(.80,.26,.14),(.07,.20,.055),black)
for i in range(12):
    y=.11-i*.012; x=.77+.025*math.sin(i*1.8)
    cyl('handset cable coil',(x,y,.12),.015,.018,black,8)
gmat=image_mat('animated gauge face atlas',P/'textures/gauges_001.png',True)
for i,(x,label) in enumerate(zip((-.18,.025,.23,.435),['FUEL','TRIM','VOLTS','RPM'])):
    cyl(label+' shallow cylinder',(x,-.10,.09),.086,.07,black,20)
    cyl(label+' rim',(x,-.10,.128),.078,.010,metal,20)
    verts=[(x,-.10,.135)]+[(x+.073*math.cos(2*math.pi*k/24),-.10+.073*math.sin(2*math.pi*k/24),.135) for k in range(24)]
    faces=[(0,k+1,(k+1)%24+1) for k in range(24)]
    mesh=bpy.data.meshes.new(label+' texture disc');mesh.from_pydata(verts,[],faces);mesh.uv_layers.new()
    for poly in mesh.polygons:
        for li in poly.loop_indices:
            v=Vector(verts[mesh.loops[li].vertex_index]);mesh.uv_layers[0].data[li].uv=((i+.5+(v.x-x)/(.073*2))/4,.5+(v.y+.10)/(.073*2))
    o=bpy.data.objects.new(label+' textured face',mesh);bpy.context.collection.objects.link(o);finish(o,o.name,gmat);o['reading_source']='illustrative demo trace; replace with runtime signal'
# Wheel is deliberately off the gauge strip, as in the approved reference.
cyl('wheel steering column',(-.64,-.29,.18),.045,.24,metal,12)
bpy.ops.mesh.primitive_torus_add(major_segments=24,minor_segments=6,location=(-.64,-.29,.32),major_radius=.225,minor_radius=.023)
finish(bpy.context.object,'steering wheel rim',black)
cyl('wheel hub',(-.64,-.29,.32),.062,.05,gold,12)
for a in (0,120,240):
    a=math.radians(a);bar('wheel spoke',(-.64,-.29,.32),(-.64+.21*math.cos(a),-.29+.21*math.sin(a),.32),.013,metal)
for j,(name,x) in enumerate(zip(['LIGHTS','HORN','WIPERS','AUX','POWER','DECK'],[-.23,-.105,.02,.145,.27,.395])):
    box(name+' rocker base',(x,-.36,.08),(.085,.12,.04),black)
    s=box(name+' rocker',(x,-.36,.108),(.06,.085,.022),metal);s['proposed_action']=name.lower();s['runtime_wired']=False
    box(name+' indicator',(x,-.335,.124),(.025,.012,.006),red)
for x in (.60,.73):
    box('throttle mounting block',(x,-.34,.11),(.11,.25,.08),black)
    lever=bar('separate throttle lever',(x,-.37,.15),(x,-.28,.32),.012,metal)
    box('throttle grip',(x,-.28,.33),(.10,.065,.045),black)
# Labels remain separate meshes for editing; render only, no invented game functions.
def text(label,x,y,z,size=.027):
    c=bpy.data.curves.new(label,'FONT');c.body=label;c.size=size;c.align_x='CENTER';o=bpy.data.objects.new(label,c);bpy.context.collection.objects.link(o);o.location=(x,y,z);finish(o,label,metal)
text('S I T K A',0,-.475,.054,.04)
for label,x in zip(['LIGHT','HORN','WIPER','AUX','PWR','DECK'],[-.23,-.105,.02,.145,.27,.395]):text(label,x,-.455,.055,.018)
scene=bpy.context.scene;scene.render.engine='CYCLES';scene.cycles.samples=16;scene.cycles.use_denoising=False
scene.render.resolution_x=1100;scene.render.resolution_y=850;scene.render.resolution_percentage=100
scene.world.color=(.25,.25,.25)
bpy.ops.mesh.primitive_plane_add(size=200,location=(0,0,-.02));floor=bpy.context.object;floor.name='review ground';floor.data.materials.append(mat('pale ground',(.16,.20,.22)))
def aim(o,p):o.rotation_euler=(Vector(p)-o.location).to_track_quat('-Z','Y').to_euler()
for loc,power,size in [((0,-3,5),500,4),((-3,-1,2),250,3),((2,2,4),400,3)]:
    bpy.ops.object.light_add(type='AREA',location=loc);o=bpy.context.object;o.data.energy=power;o.data.shape='DISK';o.data.size=size;aim(o,(0,0,.8))
bpy.ops.object.camera_add(location=(2.05,-3.7,2.65));cam=bpy.context.object;cam.data.type='ORTHO';cam.data.ortho_scale=2.65;aim(cam,(0,0,.90));scene.camera=cam
scene.view_settings.view_transform='Standard';scene.frame_start=1;scene.frame_end=24;scene.render.fps=8
scene.frame_set(1)
asset=[o for o in bpy.data.objects if o.parent==root and o.type=='MESH'];triangles=sum(sum(len(p.vertices)-2 for p in o.data.polygons) for o in asset)
(P/'manifest.json').write_text(json.dumps({'status':'physical blockout; no native integration','units':'metres','meshes':len(asset),'triangles':triangles,'gauge_animation':'24-frame 512x128 atlas, four 128px faces; no needle meshes','screen_demo':'chart/sonar alternating radar/camera','reference':'libfile_c92ddf05935481918d11566e35398077'},indent=2))
for image in bpy.data.images:
    if image.source in {'FILE','SEQUENCE'}:image.filepath='//textures/'+Path(image.filepath).name
bpy.ops.wm.save_as_mainfile(filepath=str(P/'Sitka_Maximum_Helm.blend'))
bpy.ops.object.select_all(action='DESELECT')
for o in asset:o.select_set(True)
bpy.ops.export_scene.gltf(filepath=str(P/'Sitka_Maximum_Helm.glb'),use_selection=True,export_animations=False)
scene.render.filepath=str(P/'renders/maximum_helm.png');bpy.ops.render.render(write_still=True)
scene.render.resolution_percentage=65
for f in range(1,25):
    scene.frame_set(f);scene.render.filepath=str(P/f'renders/frame_{f:03d}.png');bpy.ops.render.render(write_still=True)
print('HELM_BUILD_COMPLETE',triangles)
