#include "LitColorTextureProgram.hpp"

#include "gl_compile_program.hpp"
#include "gl_errors.hpp"

Scene::Drawable::Pipeline lit_color_texture_program_pipeline;

Load< LitColorTextureProgram > lit_color_texture_program(LoadTagEarly, []() -> LitColorTextureProgram const * {
	LitColorTextureProgram *ret = new LitColorTextureProgram();

	//----- build the pipeline template -----
	lit_color_texture_program_pipeline.program = ret->program;

	lit_color_texture_program_pipeline.OBJECT_TO_CLIP_mat4 = ret->OBJECT_TO_CLIP_mat4;
	lit_color_texture_program_pipeline.OBJECT_TO_LIGHT_mat4x3 = ret->OBJECT_TO_LIGHT_mat4x3;
	lit_color_texture_program_pipeline.NORMAL_TO_LIGHT_mat3 = ret->NORMAL_TO_LIGHT_mat3;
    lit_color_texture_program_pipeline.LIGHT_TO_SPOT_mat4 = ret->LIGHT_TO_SPOT_mat4;

    lit_color_texture_program_pipeline.USES_VERTEX_COLOR = ret->USES_VERTEX_COLOR_bool;

	/* This will be used later if/when we build a light loop into the Scene:
	lit_color_texture_program_pipeline.LIGHT_TYPE_int = ret->LIGHT_TYPE_int;
	lit_color_texture_program_pipeline.LIGHT_LOCATION_vec3 = ret->LIGHT_LOCATION_vec3;
	lit_color_texture_program_pipeline.LIGHT_DIRECTION_vec3 = ret->LIGHT_DIRECTION_vec3;
	lit_color_texture_program_pipeline.LIGHT_ENERGY_vec3 = ret->LIGHT_ENERGY_vec3;
	lit_color_texture_program_pipeline.LIGHT_CUTOFF_float = ret->LIGHT_CUTOFF_float;
	*/

    //make a 1-pixel white texture to bind by default:
    GLuint tex;
    glGenTextures(1, &tex);

    glBindTexture(GL_TEXTURE_2D, tex);
    std::vector< glm::u8vec4 > tex_data(1, glm::u8vec4(0xff));
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, tex_data.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);

    //textures:
    //texture for object
    lit_color_texture_program_pipeline.textures[0].texture = tex;
    lit_color_texture_program_pipeline.textures[0].target = GL_TEXTURE_2D;

    GL_ERRORS();
	return ret;
});

LitColorTextureProgram::LitColorTextureProgram() {
    //forward lighting shader based on https://github.com/15-466/15-466-f19-base6/blob/master/BasicMaterialForwardProgram.cpp
    //shadow mapping based on https://github.com/ixchow/15-466-f18-base3/blob/master/texture_program.cpp
    //shader with no vertex color
    program = gl_compile_program(
            //vertex shader:
            "#version 330\n"
            "uniform mat4 OBJECT_TO_CLIP;\n"
            "uniform mat4x3 OBJECT_TO_LIGHT;\n"
            "uniform mat3 NORMAL_TO_LIGHT;\n"
            "uniform mat4 LIGHT_TO_SPOT;\n"
            "in vec4 Position;\n"
            "in vec3 Normal;\n"
            "in vec4 Color;\n"
            "in vec2 TexCoord;\n"
            "out vec3 position;\n"
            "out vec3 normal;\n"
            "out vec4 color;\n"
            "out vec2 texCoord;\n"
            "out vec4 spotPosition;\n"
            "void main() {\n"
            "	gl_Position = OBJECT_TO_CLIP * Position;\n"
            "	position = OBJECT_TO_LIGHT * Position;\n"
            "   spotPosition = LIGHT_TO_SPOT * vec4(position, 1.0f); \n"
            "	normal = NORMAL_TO_LIGHT * Normal;\n"
            "	color = Color;\n"
            "	texCoord = TexCoord;\n"
            "}\n"
            ,
            //fragment shader:
            "#version 330\n"
            "uniform sampler2D TEX;\n"
            "uniform sampler2D DEPTH_TEX;\n"
            "uniform sampler2DShadow DIRECTIONAL_DEPTH_TEX;\n"
            "uniform float ROUGHNESS;\n"
            "uniform uint LIGHTS;\n"
            "uniform int LIGHT_TYPE[" + std::to_string(MaxLights) + "];\n"
            "uniform vec3 LIGHT_LOCATION[" + std::to_string(MaxLights) + "];\n"
            "uniform vec3 LIGHT_DIRECTION[" + std::to_string(MaxLights) + "];\n"
            "uniform vec3 LIGHT_ENERGY[" + std::to_string(MaxLights) + "];\n"
            "uniform float LIGHT_CUTOFF[" + std::to_string(MaxLights) + "];\n"
            "uniform bool USES_VERTEX_COLOR;\n"
            "uniform vec3 EYE;\n"
            "in vec3 position;\n"
            "in vec3 normal;\n"
            "in vec4 color;\n"
            "in vec2 texCoord;\n"
            "in vec4 spotPosition;\n"
            "out vec4 fragColor;\n"
            "void main() {\n"
            "	float shininess = pow(1024.0, 1.0 - ROUGHNESS);\n"
            "	vec4 albedo;\n"
            "	vec3 v = normalize(EYE - position);\n"
            "	vec3 total = vec3(0.0f); //total light output\n"
            "	vec3 n = normalize(normal);\n"
            "	vec3 e;\n"
            "   if (USES_VERTEX_COLOR) {\n"
            "       albedo = texture(TEX, texCoord) * color;\n"
            "   } else {\n"
            "       albedo = texture(TEX, texCoord);\n"
            "   }\n"
            "   if (albedo.a < 0.5) discard;\n"
            "	for (uint light = 0u; light < LIGHTS; ++light) {\n"
            "		int TYPE = LIGHT_TYPE[light];\n"
            "		vec3 LOCATION = LIGHT_LOCATION[light];\n"
            "		vec3 DIRECTION = LIGHT_DIRECTION[light];\n"
            "		vec3 ENERGY = LIGHT_ENERGY[light];\n"
            "		float CUTOFF = LIGHT_CUTOFF[light];\n"
            "		vec3 l; //direction to light\n"
            "		vec3 h; //half-vector\n"
            "		vec3 e; //light flux\n"
            "		if (TYPE == 0) { //point light \n"
            "			l = (LOCATION - position);\n"
            "			float dis2 = dot(l,l);\n"
            "			l = normalize(l);\n"
            "			h = normalize(l+v);\n"
            "			float nl = max(0.0, dot(n, l)) / max(1.0, dis2);\n"
            "			e = nl * ENERGY;\n"
            "		} else if (TYPE == 1) { //hemi light \n"
            "			l = -DIRECTION;\n"
            "			h = vec3(0.0); //no specular from hemi for now\n"
            "			e = (dot(n,l) * 0.5 + 0.5) * ENERGY;\n"
            "		} else if (TYPE == 2) { //spot light \n"
            "			l = (LOCATION - position);\n"
            "			float dis2 = dot(l,l);\n"
            "			l = normalize(l);\n"
            "			h = normalize(l+v);\n"
            "			float nl = max(0.0, dot(n, l)) / max(1.0, dis2);\n"
            "			float c = dot(l,-DIRECTION);\n"
            "			nl *= smoothstep(CUTOFF,mix(CUTOFF,1.0,0.1), c);\n"
            "			e = nl * ENERGY;\n"
            "		} else { //(TYPE == 3) //directional light \n"
            "			l = -DIRECTION;\n"
            "           float shadow = 0.0; //primitive soft shadow implementation based on https://learnopengl.com/Advanced-Lighting/Shadows/Shadow-Mapping\n"
            "           float d = 0.001f;\n"
            "           for(int x = -1; x <= 1; ++x)\n"
            "           {\n"
            "               for(int y = -1; y <= 1; ++y)\n"
            "               {\n"
            "                   shadow += textureProj(DIRECTIONAL_DEPTH_TEX, spotPosition + vec4(x * d, y * d, 0.0, 0.0));      \n"
            "               }    \n"
            "           }\n"
            "           shadow /= 9.0;\n"
            "			h = normalize(l+v) * shadow;\n"
            "			e = max(0.0, dot(n,l)) * ENERGY * shadow;\n"
            "		}\n"
            "		vec3 reflectance =\n"
            "			albedo.rgb / 3.1415926 //Lambertian Diffuse\n"
            "			+ pow(max(0.0, dot(n, h)), shininess) //Blinn-Phong Specular\n"
            "			  * (shininess + 2.0) / (8.0) //normalization factor\n"
            "			  * mix(0.04, 1.0, pow(1.0 - max(0.0, dot(h, v)), 5.0)) //Schlick's approximation for Fresnel reflectance\n"
            "		;\n"
            "		total += e * reflectance;\n"
            "	}\n"
            "	fragColor = vec4(total, albedo.a);\n"
            "}\n"
    );

	//As you can see above, adjacent strings in C/C++ are concatenated.
	// this is very useful for writing long shader programs inline.

	//look up the locations of vertex attributes:
	Position_vec4 = glGetAttribLocation(program, "Position");
	Normal_vec3 = glGetAttribLocation(program, "Normal");
	Color_vec4 = glGetAttribLocation(program, "Color");
	TexCoord_vec2 = glGetAttribLocation(program, "TexCoord");

	//look up the locations of uniforms:
	OBJECT_TO_CLIP_mat4 = glGetUniformLocation(program, "OBJECT_TO_CLIP");
	OBJECT_TO_LIGHT_mat4x3 = glGetUniformLocation(program, "OBJECT_TO_LIGHT");
	NORMAL_TO_LIGHT_mat3 = glGetUniformLocation(program, "NORMAL_TO_LIGHT");
    LIGHT_TO_SPOT_mat4 = glGetUniformLocation(program, "LIGHT_TO_SPOT");

    ROUGHNESS_float = glGetUniformLocation(program, "ROUGHNESS");
    EYE_vec3 = glGetUniformLocation(program, "EYE");
    LIGHTS_uint = glGetUniformLocation(program, "LIGHTS");

	LIGHT_TYPE_int_array = glGetUniformLocation(program, "LIGHT_TYPE");
	LIGHT_LOCATION_vec3_array = glGetUniformLocation(program, "LIGHT_LOCATION");
	LIGHT_DIRECTION_vec3_array = glGetUniformLocation(program, "LIGHT_DIRECTION");
	LIGHT_ENERGY_vec3_array = glGetUniformLocation(program, "LIGHT_ENERGY");
	LIGHT_CUTOFF_float_array = glGetUniformLocation(program, "LIGHT_CUTOFF");

    USES_VERTEX_COLOR_bool = glGetUniformLocation(program, "USES_VERTEX_COLOR");


	GLuint TEX_sampler2D = glGetUniformLocation(program, "TEX");
    GLuint DIRECTIONAL_DEPTH_TEX_sampler2D = glGetUniformLocation(program, "DIRECTIONAL_DEPTH_TEX");

	//set TEX to always refer to texture binding zero:
	glUseProgram(program); //bind program -- glUniform* calls refer to this program now

	glUniform1i(TEX_sampler2D, 0); //set TEX to sample from GL_TEXTURE0
    glUniform1i(DIRECTIONAL_DEPTH_TEX_sampler2D, 5); //set DIRECTIONAL_DEPTH_TEX_sampler2D to sample from GL_TEXTURE5 (1-4 reserved for per-drawable textures)

	glUseProgram(0); //unbind program -- glUniform* calls refer to ??? now
}

LitColorTextureProgram::~LitColorTextureProgram() {
	glDeleteProgram(program);
	program = 0;
}

