//From https://github.com/ixchow/15-466-f18-base3/
#include "depth_program.hpp"

#include "gl_compile_program.hpp"
#include "gl_errors.hpp"
#include "BoneLitColorTextureProgram.hpp"

Scene::Drawable::Pipeline shadow_program_pipeline;

ShadowProgram::ShadowProgram() {
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

Load< ShadowProgram > shadow_program(LoadTagEarly, [](){
	ShadowProgram *ret = new ShadowProgram();
    shadow_program_pipeline.OBJECT_TO_CLIP_mat4 = ret->OBJECT_TO_CLIP_mat4;
    shadow_program_pipeline.OBJECT_TO_LIGHT_mat4x3 = ret->OBJECT_TO_LIGHT_mat4x3;
    shadow_program_pipeline.program = ret->program;

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
    shadow_program_pipeline.textures[0].texture = tex;
    shadow_program_pipeline.textures[0].target = GL_TEXTURE_2D;

    GL_ERRORS();
    return ret;
});

//Shadow program for creatures
Scene::Drawable::Pipeline bone_shadow_program_pipeline;

BoneShadowProgram::BoneShadowProgram() {

    program = gl_compile_program(
            "#version 330\n"
            "uniform mat4 object_to_clip;\n"
            "uniform mat4x3 object_to_light;\n"
            "uniform mat4x3 BONES[" + std::to_string(BoneLitColorTextureProgram::MaxBones) + "];\n"
            "layout(location=0) in vec4 Position;\n" //note: layout keyword used to make sure that the location-0 attribute is always bound to something
            //		"in vec3 Normal;\n" //DEBUG
            "in vec2 TexCoord;\n"
            "in vec4 BoneWeights;\n"
            "in uvec4 BoneIndices;\n"
            //		"out vec3 color;\n" //DEBUG
            "out vec2 texCoord;\n"
            "out vec4 position;\n"
            "void main() {\n"
            "	vec3 blended_Position = (\n"
            "		(BONES[BoneIndices.x] * Position) * BoneWeights.x\n"
            "		+ (BONES[BoneIndices.y] * Position) * BoneWeights.y\n"
            "		+ (BONES[BoneIndices.z] * Position) * BoneWeights.z\n"
            "		+ (BONES[BoneIndices.w] * Position) * BoneWeights.w\n"
            "		);\n"
            "	gl_Position = object_to_clip * vec4(blended_Position, 1.0);\n"
            "	position = mat4(object_to_light) * vec4(blended_Position, 1.0);\n"
            "	texCoord = TexCoord;\n"
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


    BoneWeights_vec4 = glGetAttribLocation(program, "BoneWeights");
    BoneIndices_uvec4 = glGetAttribLocation(program, "BoneIndices");
    BONES_mat4x3_array = glGetUniformLocation(program, "BONES");

    OBJECT_TO_CLIP_mat4 = glGetUniformLocation(program, "object_to_clip");
    OBJECT_TO_LIGHT_mat4x3 = glGetUniformLocation(program, "object_to_light");
    GLuint TEX_sampler2D = glGetUniformLocation(program, "TEX");


    glUseProgram(program); //bind program
    glUniform1i(TEX_sampler2D, 0); //set TEX to sample from GL_TEXTURE0

    glUseProgram(0); //unbind program
}

Load< BoneShadowProgram > bone_shadow_program(LoadTagEarly, [](){
    BoneShadowProgram *ret = new BoneShadowProgram();
    bone_shadow_program_pipeline.OBJECT_TO_CLIP_mat4 = ret->OBJECT_TO_CLIP_mat4;
    bone_shadow_program_pipeline.OBJECT_TO_LIGHT_mat4x3 = ret->OBJECT_TO_LIGHT_mat4x3;
    bone_shadow_program_pipeline.program = ret->program;

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
    bone_shadow_program_pipeline.textures[0].texture = tex;
    bone_shadow_program_pipeline.textures[0].target = GL_TEXTURE_2D;

    GL_ERRORS();
    return ret;
});
//
//Scene::Drawable::Pipeline prepass_program_pipeline;
//
//PrepassProgram::PrepassProgram() {
//    //note: returns world position
//    program = gl_compile_program(
//            "#version 330\n"
//            "uniform mat4 object_to_clip;\n"
//            "uniform mat4x3 object_to_light;\n"
//            "layout(location=0) in vec4 Position;\n" //note: layout keyword used to make sure that the location-0 attribute is always bound to something
//            "in vec3 Normal;\n" //DEBUG
//            "in vec2 TexCoord;\n"
//            //		"out vec3 color;\n" //DEBUG
//            "out vec2 texCoord;\n"
//            "out vec4 position;\n"
//            "void main() {\n"
//            "	gl_Position = mat4(object_to_clip) * Position;\n"
//            "   position = mat4(object_to_light) * Position;\n"
//            "   texCoord = TexCoord;\n"
//            //		"	color = 0.5 + 0.5 * Normal;\n" //DEBUG
//            "}\n"
//            ,
//            "#version 330\n"
//            //		"in vec3 color;\n" //DEBUG
//            "in vec2 texCoord;\n"
//            "in vec4 position;\n"
//            "uniform sampler2D TEX;\n"
//            "out vec4 fragColor;\n"
//            "void main() {\n"
//            "   vec4 albedo = texture(TEX, texCoord);\n"
//            "   if(albedo.a < 0.5) {\n"
//            "       discard;\n"
//            "   }\n"
//            "	fragColor = position;\n"
//            "}\n"
//    );
//
//    OBJECT_TO_CLIP_mat4 = glGetUniformLocation(program, "object_to_clip");
//    OBJECT_TO_LIGHT_mat4x3 = glGetUniformLocation(program, "object_to_light");
//    GLuint TEX_sampler2D = glGetUniformLocation(program, "TEX");
//
//
//    glUseProgram(program); //bind program
//    glUniform1i(TEX_sampler2D, 0); //set TEX to sample from GL_TEXTURE0
//
//    glUseProgram(0); //unbind program
//}
//
//Load< PrepassProgram > prepass_program(LoadTagEarly, [](){
//    PrepassProgram *ret = new PrepassProgram();
//    prepass_program_pipeline.OBJECT_TO_CLIP_mat4 = ret->OBJECT_TO_CLIP_mat4;
//    prepass_program_pipeline.OBJECT_TO_LIGHT_mat4x3 = ret->OBJECT_TO_LIGHT_mat4x3;
//    prepass_program_pipeline.program = ret->program;
//
//    //make a 1-pixel white texture to bind by default:
//    GLuint tex;
//    glGenTextures(1, &tex);
//
//    glBindTexture(GL_TEXTURE_2D, tex);
//    std::vector< glm::u8vec4 > tex_data(1, glm::u8vec4(0xff));
//    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, tex_data.data());
//    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
//    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
//    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
//    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
//    glBindTexture(GL_TEXTURE_2D, 0);
//
//    //textures:
//    //texture for object
//    shadow_program_pipeline.textures[0].texture = tex;
//    shadow_program_pipeline.textures[0].target = GL_TEXTURE_2D;
//
//    GL_ERRORS();
//    return ret;
//});


