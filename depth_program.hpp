//From https://github.com/ixchow/15-466-f18-base3/
#include "GL.hpp"
#include "Load.hpp"
#include "Scene.hpp"

struct DepthProgram {
	//opengl program object:
	GLuint program = 0;

	//uniform locations:
	GLuint object_to_clip_mat4 = -1U;

	DepthProgram();
};

extern Load< DepthProgram > depth_program;

extern Scene::Drawable::Pipeline depth_program_pipeline;
