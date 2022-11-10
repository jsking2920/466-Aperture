:: Scott King (10/31/22)
:: Runs blender asset pipeline and builds game.exe
:: All references to blender need to be swapped to Blender 3.2 for this to work on my laptop or 2.93 to work on my desktop
@ECHO OFF
ECHO Running pipeline and building game.exe
ECHO ==============================================
Echo Exporting meshes...
"C:\Program Files\Blender Foundation\Blender 3.2\blender.exe" --background --python scenes/export-meshes.py -- scenes/proto-world2.blend dist/assets/proto-world2.pnct
ECHO ==============================================
Echo Exporting walkmesh...
"C:\Program Files\Blender Foundation\Blender 3.2\blender.exe" --background --python scenes/export-walkmeshes.py -- scenes/proto-world2.blend:WalkMeshes dist/assets/proto-world2.w
ECHO ==============================================
Echo Exporting scene...
"C:\Program Files\Blender Foundation\Blender 3.2\blender.exe" --background --python scenes/export-scene.py -- scenes/proto-world2.blend dist/assets/proto-world2.scene
ECHO ==============================================
Echo Running maekfile...
node Maekfile.js -j1 &
ECHO ==============================================
Echo All done! Starting game
.\dist\game.exe