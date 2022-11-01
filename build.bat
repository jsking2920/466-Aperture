:: Scott King (10/31/22)
:: Runs blender asset pipeline and builds game.exe
@ECHO OFF
ECHO Running pipeline and building game.exe
ECHO ==============================================
Echo Exporting meshes...
"C:\Program Files\Blender Foundation\Blender 2.93\blender.exe" --background --python scenes/export-meshes.py -- scenes/proto-world.blend dist/proto-world.pnct
ECHO ==============================================
Echo Exporting walkmesh...
"C:\Program Files\Blender Foundation\Blender 2.93\blender.exe" --background --python scenes/export-walkmeshes.py -- scenes/proto-world.blend:WalkMeshes dist/proto-world.w
ECHO ==============================================
Echo Exporting scene...
"C:\Program Files\Blender Foundation\Blender 2.93\blender.exe" --background --python scenes/export-scene.py -- scenes/proto-world.blend dist/proto-world.scene
ECHO ==============================================
Echo Running maekfile...
node Maekfile.js -j1 &
ECHO ==============================================
Echo All done! Starting game
.\dist\game.exe