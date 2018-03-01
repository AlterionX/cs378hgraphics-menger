#include "ship.h"

#include "fluid.h"
#include "sphere.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/string_cast.hpp>
#include <iostream>

void ship::generate_geometry(std::vector<glm::vec4>& obj_vertices, std::vector<glm::uvec3>& obj_faces) {
    //TODO load obj file
    sphere::create_sphere(1.0, 15, 15, obj_vertices, obj_faces);
}
glm::mat4 ship::model_matrix(double t, ship::instance& inst, fluid::ocean_surf_params params) {
    auto base = glm::mat4(1.0f);
    auto plane_pos = glm::vec2 { inst.w_pos[0], inst.w_pos[1] };

    base = glm::translate(base, glm::vec3(fluid::simulate_offset(t, plane_pos, params) + inst.w_pos));

    auto norm = glm::vec3(fluid::simulate_normal(t, plane_pos, params));
    auto rot_axis = glm::cross(inst.up, glm::normalize(norm));
    auto rot_theta = glm::acos(glm::dot(inst.up, glm::normalize(norm)));
    base = glm::rotate(base, rot_theta, rot_axis);

    return base;
}
void ship::instance::simulate(double dt, std::vector<ship::instance>& fellow_ships) {
    // TODO change from no op
}
