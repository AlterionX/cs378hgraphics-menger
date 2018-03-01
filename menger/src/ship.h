#ifndef __SHIP_H__
#define __SHIP_H__

#include <glm/glm.hpp>
#include <vector>

#include "fluid.h"

namespace ship {
    struct instance {
        bool side;
        glm::vec4 w_pos;
        glm::vec4 vel;
        glm::vec4 acl;
        glm::vec3 up;
        glm::vec3 forw;
        double t_scale = 1000000.0;
        void simulate(double dt, std::vector<instance>& fellow_ships);
    };
    void generate_geometry(std::vector<glm::vec4>& obj_vertices, std::vector<glm::uvec3>& obj_faces);
    glm::mat4 model_matrix(double t, instance& inst, fluid::ocean_surf_params params);
}

#endif
