
## Helpful
"C:\Program Files\Blender Foundation\Blender 3.3\blender.exe" --background --python scenes\export-scene.py -- scenes\proto-world2.blend dist\assets\proto-world2.scene


"C:\Program Files\Blender Foundation\Blender 3.3\blender.exe" --background --python scenes\export-meshes.py -- scenes\proto-world2.blend dist\assets\proto-world2.pnct

"C:\Program Files\Blender Foundation\Blender 3.3\blender.exe" --background --python scenes\export-walkmeshes.py -- scenes\proto-world2.blend:WalkMeshes dist\assets\proto-world2.w

//big ass command
"C:\Program Files\Blender Foundation\Blender 3.3\blender.exe" --background --python scenes\export-scene.py -- scenes\proto-world2.blend dist\assets\proto-world2.scene && "C:\Program Files\Blender Foundation\Blender 3.3\blender.exe" --background --python scenes\export-meshes.py -- scenes\proto-world2.blend dist\assets\proto-world2.pnct && "C:\Program Files\Blender Foundation\Blender 3.3\blender.exe" --background --python scenes\export-walkmeshes.py -- scenes\proto-world2.blend:WalkMeshes dist\assets\proto-world2.w

//export animation 
"C:\Program Files\Blender Foundation\Blender 3.3\blender.exe" --background --python scenes\export-bone-animations.py -- scenes\proto-world2.blend Armature [0,60]Test!first dist\assets\testanim.banims

"C:\Program Files\Blender Foundation\Blender 3.3\blender.exe" --background --python scenes\export-bone-animations.py -- scenes\proto-world2.blend ARM_FLO_02 [0,40]Test!global dist\assets\testanim.banims

"C:\Program Files\Blender Foundation\Blender 3.3\blender.exe" --background --python scenes\export-bone-animations.py -- scenes\fixinganimation.blend Armature [0,40]Idle!global;[40,80]Action1!global dist\assets\testanim.banims


"C:\Program Files\Blender Foundation\Blender 3.3\blender.exe" --background --python scenes\export-bone-animations.py -- scenes\proto-world2.blend Fuck [0,30]Test!global dist\assets\monkey.banims


blender --background --python export-bone-animations.py -- <infile.blend> <object> <action[;action2][;...]> <outfile.character>
Exports an armature-animated mesh to a binary blob.
<action> can also be a named frame range as per '[100,150]Walk'
<action> can specify root transforms by appending 'Walk!local' (local to armature),'Walk!global' (world-relative),'Walk!first' (first-frame relative)

blender --background --python scenes/export-meshes.py -- scenes/proto-world.blend dist/proto-world.pnct
blender --background --python scenes/export-walkmeshes.py -- scenes/proto-world.blend:WalkMeshes dist/proto-world.w 

C:\Program Files\Blender Foundation\Blender 3.3

thinking about... importing animations
Ideally we want the animation to be binded to the creatures, such that we can play them at will...
Alternatively, we have a huge look up table that's creature to animation...
Each animation file corresponds with one skeleton -- can they share animations?

there's a list of currently playing animations, getting updated
Each 
