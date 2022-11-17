//From https://github.com/ixchow/15-466-f18-base3/
#include "GL.hpp"
#include "Load.hpp"
#include "Scene.hpp"

struct DepthProgram {
	//opengl program object:
	GLuint program = 0;

	//uniform locations:
	GLuint OBJECT_TO_CLIP_mat4 = -1U;

    //textures
    //0 - object texture

	DepthProgram();
};

extern Load< DepthProgram > depth_program;

extern Scene::Drawable::Pipeline depth_program_pipeline;
