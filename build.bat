:: Scott King (10/31/22)
:: Runs blender asset pipeline and builds game.exe

:: Notes
:: All references to blender need to be swapped to Blender 3.2 for this to work on my laptop or 2.93 to work on my desktop
:: to be run from path/to/466-Aperture/ 
:: in the x64 Native Tools Command Prompt for VS 2022

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
node Maekfile.js -j1
ECHO ==============================================

Echo All done! Starting game
.\dist\game.exe



:: Some other possible commands
:: Non-woring script for cleaning up transparent pixels in textures: python3 ./scenes/clean-textures.py ./dist/assets/textures/
:: Must have SDL2.dll in same folder with these .exe's for them to the run
:: .\dist\show-meshes.exe .\dist\assets\proto-world2.pnct
:: .\dist\show-scene.exe .\dist\assets\proto-world2.scene .\dist\assets\proto-world2.pnct

:: Sprite pipeline
:: See README in /sprites/ for details
:: python3 sprites/extract-sprites.py sprites/ui_sprites.list dist/assets/sprites/ --gimp="C:\Program Files\GIMP 2\bin\gimp-2.10.exe"
:: python3 sprites/extract-sprites.py sprites/the-planet.list dist/assets/sprites/ --gimp="C:\Program Files\GIMP 2\bin\gimp-2.10.exe"