//From https://github.com/ixchow/15-466-f18-base3/
#include "GL.hpp"
#include "Load.hpp"
#include "Scene.hpp"

struct ShadowProgram {
	//opengl program object:
	GLuint program = 0;

	//uniform locations:
	GLuint OBJECT_TO_CLIP_mat4 = -1U;
    GLuint OBJECT_TO_LIGHT_mat4x3 = -1U;

    //textures
    //0 - object texture

	ShadowProgram();
};

extern Load< ShadowProgram > shadow_program;

extern Scene::Drawable::Pipeline shadow_program_pipeline;

struct PrepassProgram {
    //opengl program object:
    GLuint program = 0;

    //uniform locations:
    GLuint OBJECT_TO_CLIP_mat4 = -1U;
    GLuint OBJECT_TO_LIGHT_mat4x3 = -1U;

    //textures
    //0 - object texture

    PrepassProgram();
};

extern Load< PrepassProgram > prepass_program;

extern Scene::Drawable::Pipeline prepass_program_pipeline;