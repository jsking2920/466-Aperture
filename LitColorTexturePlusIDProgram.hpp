#pragma once

// Based on LitColorTextureProgram from base code by Jim McCann
// and this: https://github.com/textiles-lab/autoknit/blob/0021de31fbf0478668b898673c63c736c23216cb/Interface.cpp#L81-L94 

#include "GL.hpp"
#include "Load.hpp"
#include "Scene.hpp"

//Shader program that draws transformed, lit, textured vertices tinted with vertex colors:
struct LitColorTexturePlusIDProgram {
	LitColorTexturePlusIDProgram();
	~LitColorTexturePlusIDProgram();

	GLuint program = 0;

	//Attribute (per-vertex variable) locations:
	GLuint Position_vec4 = -1U;
	GLuint Normal_vec3 = -1U;
	GLuint Color_vec4 = -1U;
	GLuint TexCoord_vec2 = -1U;

	//Uniform (per-invocation variable) locations:
	GLuint OBJECT_TO_CLIP_mat4 = -1U;
	GLuint OBJECT_TO_LIGHT_mat4x3 = -1U;
	GLuint NORMAL_TO_LIGHT_mat3 = -1U;

	//lighting:
	GLuint LIGHT_TYPE_int = -1U;
	GLuint LIGHT_LOCATION_vec3 = -1U;
	GLuint LIGHT_DIRECTION_vec3 = -1U;
	GLuint LIGHT_ENERGY_vec3 = -1U;
	GLuint LIGHT_CUTOFF_float = -1U;

	//Textures:
	//TEXTURE0 - texture that is accessed by TexCoord
};

extern Load< LitColorTexturePlusIDProgram > lit_color_texture_plus_id_program;

//For convenient scene-graph setup, copy this object:
// NOTE: by default, has texture bound to 1-pixel white texture -- so it's okay to use with vertex-color-only meshes.
extern Scene::Drawable::Pipeline lit_color_texture_plus_id_program_pipeline;
