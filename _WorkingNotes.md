
## Helpful
Animation Notes:
TAN idle 0-60 action 60-120
TRI idel 0-40 action 40 - 80
SNA idle 0-40 action 40-80
PEN idle 0-60 action 60-90
MEP idle 0-60 action 60-120


expected behavior?
TAN - think whales: hovers above ground, moving slowly, growls if you take a picture of it
TRI - angry goose: stays stationary on the ground, runs/drifts towareds the player agressively
SNA - crawls slowly on the ground, hides in shell once close
PEN - this is the penguin equivalent in our game! waddles around, flaps wings randomly?
MEP - more active floater? 

//big ass command
"C:\Program Files\Blender Foundation\Blender 3.3\blender.exe" --background --python scenes\export-scene.py -- scenes\proto-world2.blend dist\assets\proto-world2.scene && "C:\Program Files\Blender Foundation\Blender 3.3\blender.exe" --background --python scenes\export-meshes.py -- scenes\proto-world2.blend dist\assets\proto-world2.pnct && "C:\Program Files\Blender Foundation\Blender 3.3\blender.exe" --background --python scenes\export-walkmeshes.py -- scenes\proto-world2.blend:WalkMeshes dist\assets\proto-world2.w

//export animation 
"C:\Program Files\Blender Foundation\Blender 3.3\blender.exe" --background --python scenes\export-bone-animations.py -- scenes\proto-world2.blend Armature [0,60]Test!first dist\assets\testanim.banims

"C:\Program Files\Blender Foundation\Blender 3.3\blender.exe" --background --python scenes\export-bone-animations.py -- scenes\proto-world2.blend ARM_FLO_02 [0,40]Test!global dist\assets\testanim.banims

"C:\Program Files\Blender Foundation\Blender 3.3\blender.exe" --background --python scenes\export-bone-animations.py -- scenes\fixinganimation.blend Armature [0,40]Idle!global;[40,80]Action1!global dist\assets\testanim.banims

//EXPORT SCRIPTS --------------------
"C:\Program Files\Blender Foundation\Blender 3.3\blender.exe" --background --python scenes\export-bone-animations.py -- scenes\CREAnimations.blend ARM_MEP_01 [0,60]Idle!global;[60,120]Action1!global dist\assets\animations\anim_MEP.banims

"C:\Program Files\Blender Foundation\Blender 3.3\blender.exe" --background --python scenes\export-bone-animations.py -- scenes\CREAnimations.blend ARM_PEN_01 [0,60]Idle!global;[60,90]Action1!global dist\assets\animations\anim_PEN.banims

"C:\Program Files\Blender Foundation\Blender 3.3\blender.exe" --background --python scenes\export-bone-animations.py -- scenes\CREAnimations.blend ARM_SNA_01 [0,40]Idle!global;[40,80]Action1!global dist\assets\animations\anim_SNA.banims

"C:\Program Files\Blender Foundation\Blender 3.3\blender.exe" --background --python scenes\export-bone-animations.py -- scenes\CREAnimations.blend ARM_TAN_01 [0,60]Idle!global;[60,120]Action1!global dist\assets\animations\anim_TAN.banims

"C:\Program Files\Blender Foundation\Blender 3.3\blender.exe" --background --python scenes\export-bone-animations.py -- scenes\CREAnimations.blend ARM_TRI_01 [0,40]Idle!global;[40,80]Action1!global dist\assets\animations\anim_TRI.banims


//---------------
    

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
