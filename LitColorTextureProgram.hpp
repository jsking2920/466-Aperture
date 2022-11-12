#pragma once

#include "GL.hpp"
#include "Load.hpp"
#include "Scene.hpp"

//Shader program that draws transformed, lit, textured vertices tinted with vertex colors:
struct LitColorTextureProgram {
	LitColorTextureProgram();
	~LitColorTextureProgram();

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

	//lighting: based on https://github.com/15-466/15-466-f19-base6/blob/master/BasicMaterialForwardProgram.hpp
    GLuint EYE_vec3 = -1U; //camera position in lighting space
    GLuint LIGHTS_uint = -1U;
    GLfloat ROUGHNESS_float = -1U;

    GLuint LIGHT_TYPE_int_array = -1U;
    GLuint LIGHT_LOCATION_vec3_array = -1U;
    GLuint LIGHT_DIRECTION_vec3_array = -1U;
    GLuint LIGHT_ENERGY_vec3_array = -1U;
    GLuint LIGHT_CUTOFF_float_array = -1U;

    enum : uint32_t { MaxLights = 40 };
	
	//Textures:
	//TEXTURE0 - texture that is accessed by TexCoord
    GLuint USES_VERTEX_COLOR_bool = -1U;
};

extern Load< LitColorTextureProgram > lit_color_texture_program;

//For convenient scene-graph setup, copy this object:
extern Scene::Drawable::Pipeline lit_color_texture_program_pipeline;
