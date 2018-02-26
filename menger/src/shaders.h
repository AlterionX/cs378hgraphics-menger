#include <GL/glew.h>
#include <GLFW/glfw3.h>

namespace shaders {
    /**
    *** List of pipelines:
    ***     menger cube
    ***     wireframe
    ***     ocean
    ***     floor
    **/

    class GLSPS; // forward declare

    // Since we're not too concerned about efficiency
    class GLSSS { // OpenGL shader source struct
    public:
        GLSSS(const char* vs, const char* tcs, const char* tes, const char* gs, const char* fs);

        const char* vs(void) const;
        const char* tcs(void) const;
        const char* tes(void) const;
        const char* gs(void) const;
        const char* fs(void) const;
        const char* ss(const int i) const;

        GLSPS compile(void) const;

        static GLuint type(const int i);

    private:
        const char* ss_data[5];
    };

    extern GLSSS menger_sss;
    extern GLSSS floor_sss;
    extern GLSSS ocean_sss;

    class GLSPS { // OpenGL shader program struct
    public:
        GLSPS(const GLSSS* const sss);

        GLuint vs_id(void) const;
        GLuint tcs_id(void) const;
        GLuint tes_id(void) const;
        GLuint gs_id(void) const;
        GLuint fs_id(void) const;
        GLuint sp_id(const int i) const;

        GLuint create_program() const;

    private:
        GLuint sp_ids[5] {0, 0, 0, 0, 0};
    };
}
