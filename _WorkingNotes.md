
## Helpful
"C:\Program Files\Blender Foundation\Blender 3.3\blender.exe" --background --python export-scene.py -- proto-world.blend proto-world.scene


"C:\Program Files\Blender Foundation\Blender 3.3\blender.exe" --background --python export-meshes.py -- proto-world.blend proto-world.pnct

"C:\Program Files\Blender Foundation\Blender 3.3\blender.exe" --background --python export-walkmeshes.py -- proto-world.blend:WalkMeshes proto-world.w


"C:\Program Files\Blender Foundation\Blender 3.3\blender.exe" --background --python export-scene.py -- proto-world2.blend proto-world2.scene


"C:\Program Files\Blender Foundation\Blender 3.3\blender.exe" --background --python export-meshes.py -- proto-world2.blend proto-world2.pnct

"C:\Program Files\Blender Foundation\Blender 3.3\blender.exe" --background --python export-walkmeshes.py -- proto-world2.blend:WalkMeshes proto-world2.w

//export animation 
"C:\Program Files\Blender Foundation\Blender 3.3\blender.exe" --background --python scenes/export-bone-animations.py -- scenes/proto-world2.blend Armature [0,60]Test!local testanim.character




blender --background --python export-bone-animations.py -- <infile.blend> <object> <action[;action2][;...]> <outfile.character>
Exports an armature-animated mesh to a binary blob.
<action> can also be a named frame range as per '[100,150]Walk'
<action> can specify root transforms by appending 'Walk!local' (local to armature),'Walk!global' (world-relative),'Walk!first' (first-frame relative)

blender --background --python scenes/export-meshes.py -- scenes/proto-world.blend dist/proto-world.pnct
blender --background --python scenes/export-walkmeshes.py -- scenes/proto-world.blend:WalkMeshes dist/proto-world.w 

C:\Program Files\Blender Foundation\Blender 3.3