
## Helpful
"C:\Program Files\Blender Foundation\Blender 3.3\blender.exe" --background --python export-scene.py -- proto-world.blend proto-world.scene


"C:\Program Files\Blender Foundation\Blender 3.3\blender.exe" --background --python export-meshes.py -- proto-world.blend proto-world.pnct

"C:\Program Files\Blender Foundation\Blender 3.3\blender.exe" --background --python export-walkmeshes.py -- proto-world.blend:WalkMeshes proto-world.w


"C:\Program Files\Blender Foundation\Blender 3.3\blender.exe" --background --python export-scene.py -- proto-world2.blend proto-world2.scene


"C:\Program Files\Blender Foundation\Blender 3.3\blender.exe" --background --python export-meshes.py -- proto-world2.blend proto-world2.pnct

"C:\Program Files\Blender Foundation\Blender 3.3\blender.exe" --background --python export-walkmeshes.py -- proto-world2.blend:WalkMeshes proto-world2.w


blender --background --python scenes/export-meshes.py -- scenes/proto-world.blend dist/proto-world.pnct
blender --background --python scenes/export-walkmeshes.py -- scenes/proto-world.blend:WalkMeshes dist/proto-world.w

C:\Program Files\Blender Foundation\Blender 3.3