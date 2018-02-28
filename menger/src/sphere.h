#ifndef __SPHERE_H__
#define __SPHERE_H__

#include <glm/glm.hpp>
#include <vector>

namespace sphere {
    void create_sphere(float radius, int spoke_cnt, int tier_cnt, std::vector<glm::vec4>& obj_vertices, std::vector<glm::uvec3>& obj_faces);
};

#endif
