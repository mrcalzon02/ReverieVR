import bpy,json
from pathlib import Path
P=Path(__file__).resolve().parent
missing=[i.filepath for i in bpy.data.images if i.source in {'FILE','SEQUENCE'} and not Path(bpy.path.abspath(i.filepath)).is_file()]
assert not missing,missing
for label in ['FUEL','TRIM','VOLTS','RPM']:
    assert bpy.data.objects[label+' shallow cylinder'].type=='MESH'
    assert bpy.data.objects[label+' textured face'].type=='MESH'
assert not [o.name for o in bpy.data.objects if 'needle' in o.name.lower()]
for frame in (1,8):
    bpy.context.scene.frame_set(frame)
    print('SCREEN_MATERIALS',frame,[o.data.materials[o.data.polygons[0].material_index].name for o in bpy.data.objects if o.name.endswith('mode surface')])
print('SAVED_BLEND_VALID',len(bpy.data.objects),'objects; all texture paths resolve')
