#include "shaders.h"
#include "shadersources.h"

#include <iostream>
#include "debuggl.h"

namespace shaders {
    GLSSS::GLSSS(const char* vs, const char* tcs, const char* tes, const char* gs, const char* fs):
        ss_data {vs, tcs, tes, gs, fs} {}

    const char* GLSSS::vs(void) const { return ss(0); }
    const char* GLSSS::tcs(void) const { return ss(1); }
    const char* GLSSS::tes(void) const { return ss(2); }
    const char* GLSSS::gs(void) const { return ss(3); }
    const char* GLSSS::fs(void) const { return ss(4); }
    const char* GLSSS::ss(const int i) const { return ss_data[i]; }
    GLuint GLSSS::type(const int i) {
        GLuint types[] = {GL_VERTEX_SHADER, GL_TESS_CONTROL_SHADER, GL_TESS_EVALUATION_SHADER, GL_GEOMETRY_SHADER, GL_FRAGMENT_SHADER};
        return types[i];
    }
    GLSPS GLSSS::compile(void) const { return GLSPS(this); }

    GLSPS::GLSPS(const GLSSS* const sss) {
        for (int i = 0; i < 5; ++i) {
            auto ss = sss->ss(i);
            sp_ids[i] = 0;
            if (ss != nullptr) {
                CHECK_GL_ERROR(sp_ids[i] = glCreateShader(GLSSS::type(i)));
                CHECK_GL_ERROR(glShaderSource(sp_ids[i], 1, &ss, nullptr));
                glCompileShader(sp_ids[i]);
                CHECK_GL_SHADER_ERROR(sp_ids[i]);
            }
        }
    }

    GLuint GLSPS::vs_id(void) const { return sp_ids[0]; }
    GLuint GLSPS::tcs_id(void) const { return sp_ids[1]; }
    GLuint GLSPS::tes_id(void) const { return sp_ids[2]; }
    GLuint GLSPS::gs_id(void) const { return sp_ids[3]; }
    GLuint GLSPS::fs_id(void) const { return sp_ids[4]; }
    GLuint GLSPS::sp_id(const int i) const { return sp_ids[i]; }

    GLuint GLSPS::create_program() const {
        GLuint program_id;
        CHECK_GL_ERROR(program_id = glCreateProgram());
        for (int i = 0; i < 5; ++i) {
            if (sp_id(i) > 0) {
                CHECK_GL_ERROR(glAttachShader(program_id, sp_id(i)));
            }
        }
        return program_id;
    }

    GLSSS menger_sss {menger_vs, nullptr, nullptr, menger_gs, menger_fs};
    GLSSS floor_sss {floor_vs, t_ctrl_shader, t_eval_shader, menger_gs, floor_fs};
    GLSSS ocean_sss {ocean_vs, quad_t_ctrl_shader, ocean_tes, ocean_gs, ocean_fs};
}
