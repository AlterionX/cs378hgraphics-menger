#include "ship.h"

#include "fluid.h"
#include "sphere.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/string_cast.hpp>

void ship::generate_geometry(std::vector<glm::vec4>& obj_vertices, std::vector<glm::uvec3>& obj_faces) {
    //TODO load obj file
    sphere::create_sphere(25, 15, 15, obj_vertices, obj_faces);
}
glm::mat4 ship::model_matrix(double t, ship::instance& inst, fluid::ocean_surf_params params) {
    auto base = glm::mat4(1.0f);
    base = glm::translate(base, glm::vec3(fluid::simulate_offset(t, inst.last_pos, params) + inst.w_pos));
    auto rot_axis = glm::cross(inst.up, inst.forw);
    auto rot_theta = glm::acos(dot(inst.up, inst.forw));
    base = glm::rotate(base, rot_theta, rot_axis);
    return base;
}
