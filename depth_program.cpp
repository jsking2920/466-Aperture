//From https://github.com/ixchow/15-466-f18-base3/
#include "depth_program.hpp"

#include "gl_compile_program.hpp"
#include "gl_errors.hpp"

Scene::Drawable::Pipeline depth_program_pipeline;

DepthProgram::DepthProgram() {
    //note: returns world position
	program = gl_compile_program(
		"#version 330\n"
		"uniform mat4 object_to_clip;\n"
        "uniform mat4x3 object_to_light;\n"
		"layout(location=0) in vec4 Position;\n" //note: layout keyword used to make sure that the location-0 attribute is always bound to something
//		"in vec3 Normal;\n" //DEBUG
        "in vec2 TexCoord;\n"
//		"out vec3 color;\n" //DEBUG
        "out vec2 texCoord;\n"
        "out vec4 position;\n"
		"void main() {\n"
		"	gl_Position = mat4(object_to_clip) * Position;\n"
        "   position = mat4(object_to_light) * Position;\n"
        "   texCoord = TexCoord;\n"
//		"	color = 0.5 + 0.5 * Normal;\n" //DEBUG
		"}\n"
		,
		"#version 330\n"
//		"in vec3 color;\n" //DEBUG
        "in vec2 texCoord;\n"
        "in vec4 position;\n"
		"uniform sampler2D TEX;\n"
		"out vec4 fragColor;\n"
		"void main() {\n"
        "   vec4 albedo = texture(TEX, texCoord);\n"
        "   if(albedo.a < 0.5) {\n"
        "       discard;\n"
        "   }\n"
		"	fragColor = position;\n"
		"}\n"
	);

    OBJECT_TO_CLIP_mat4 = glGetUniformLocation(program, "object_to_clip");
    OBJECT_TO_LIGHT_mat4x3 = glGetUniformLocation(program, "object_to_light");
    GLuint TEX_sampler2D = glGetUniformLocation(program, "TEX");


    glUseProgram(program); //bind program
    glUniform1i(TEX_sampler2D, 0); //set TEX to sample from GL_TEXTURE0

    glUseProgram(0); //unbind program
}

Load< DepthProgram > depth_program(LoadTagEarly, [](){
	DepthProgram *ret = new DepthProgram();
    depth_program_pipeline.OBJECT_TO_CLIP_mat4 = ret->OBJECT_TO_CLIP_mat4;
    depth_program_pipeline.OBJECT_TO_LIGHT_mat4x3 = ret->OBJECT_TO_LIGHT_mat4x3;
    depth_program_pipeline.program = ret->program;

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
    depth_program_pipeline.textures[0].texture = tex;
    depth_program_pipeline.textures[0].target = GL_TEXTURE_2D;

    GL_ERRORS();
    return ret;
});
