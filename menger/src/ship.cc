#include "ship.h"

#include "fluid.h"
#include "sphere.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/string_cast.hpp>
#include <iostream>
#include <fstream>
#include <sstream>

void ship::generate_geometry(std::vector<glm::vec4>& obj_vertices, std::vector<glm::uvec3>& obj_faces) {
    //TODO load obj file
    sphere::create_sphere(1.0, 15, 15, obj_vertices, obj_faces);

    std::string line_buf;
    std::ifstream boat;
    boat.open("./src/stuff.obj");
    std::vector<glm::vec4> verts;
    std::vector<glm::vec4> norms;
    std::vector<glm::vec2> tex_coord;
    std::vector<glm::uvec3> faces;

    obj_vertices.clear();
    obj_faces.clear();

    while (!boat.eof()) {
        getline(boat, line_buf);
        std::stringstream ss;
        if (line_buf.compare(0, 2, "v ") == 0) {
            ss << line_buf.substr(2);
            float x, y, z;
            ss >> x >> y >> z;
            verts.push_back(glm::vec4(x * 2.0f, y * 2.5f, z * 2.0f, 1.0f));
        } else if (line_buf.compare(0, 3, "vn ") == 0) {
            ss << line_buf.substr(3);
            float x, y, z;
            ss >> x >> y >> z;
            norms.push_back(glm::vec4(x, y, z, 0.0f));
        } else if (line_buf.compare(0, 3, "vt ") == 0) {
            ss << line_buf.substr(3);
            float u, v;
            ss >> u >> v;
            tex_coord.push_back(glm::vec2(u, v));
        } else if (line_buf.compare(0, 2, "f ") == 0) {
            ss << line_buf.substr(2);
            unsigned int v0, n0, t0, v1, n1, t1, v2, n2, t2;
            char tossaway;
            ss >> v0 >> tossaway >> t0 >> tossaway >> n0
                >> v1 >> tossaway >> t1 >> tossaway >> n1
                >> v2 >> tossaway >> t2 >> tossaway >> n2;
            faces.push_back(glm::uvec3(v0, v1, v2) - glm::uvec3(1.0));
        }
    }
    boat.close();

    obj_vertices.insert(obj_vertices.end(), verts.begin(), verts.end());
    obj_faces.insert(obj_faces.end(), faces.begin(), faces.end());

    glm::vec4 sum = glm::vec4(0.0f);
    for (auto vertex : obj_vertices) {
        sum += vertex;
        sum *= 0.5f;
    }
    sum[3] = 0.0f;
    sum[1] = 0.0f;
    for (auto& vertex : obj_vertices) {
        vertex -= sum;
    }
}
glm::mat4 ship::model_matrix(double t, ship::instance& inst, fluid::ocean_surf_params params) {
    auto base = glm::mat4(1.0f);
    auto plane_pos = glm::vec2 { inst.w_pos[0], inst.w_pos[1] };

    base = glm::translate(base, glm::vec3(fluid::simulate_offset(t, plane_pos, params) + inst.w_pos));

    auto norm = glm::normalize(glm::vec3(fluid::simulate_normal(t, plane_pos, params)));
    auto rot_axis = glm::cross(inst.up, norm);
    auto rot_theta = glm::acos(glm::dot(inst.up, norm));
    if (glm::length(rot_axis) > 0.000001) {
        base = glm::rotate(base, rot_theta, rot_axis);
    }

    auto forw_rot_axis = glm::vec3(0.0f, 1.0f, 0.0f);
    auto forw_rot_theta = glm::acos(glm::dot(inst.up, inst.forw));
    if (glm::length(forw_rot_axis) > 0.000001) {
        base = glm::rotate(base, forw_rot_theta, forw_rot_axis);
    }

    return base;
}
void ship::instance::simulate(double dt, std::vector<ship::instance>& fellow_ships) {
    // TODO change from no op
    this->w_pos += this->vel * float(dt / this->t_scale);
    this->vel += glm::vec4(this->forw * glm::dot(this->forw, glm::vec3(this->acl)), 0.0f) * float(dt / this->t_scale);
    float torque = glm::cross(this->forw, glm::vec3(this->acl))[2];
    std::cout << glm::to_string(glm::cross(this->forw, glm::vec3(this->acl))) << std::endl;
    if (torque > 0.001) {
        this->vel = glm::rotate(this->vel, torque * float(dt / this->t_scale), this->up);
    }
    this->forw = glm::normalize(this->vel);
    std::cout << glm::to_string(this->vel) << std::endl;
    if (this->w_pos[0] > 20.0f) {
        this->w_pos[0] = -20.0f;
    } else if(this->w_pos[0] < -20.0f) {
        this->w_pos[0] = 20.0f;
    }
    if (this->w_pos[2] > 20.0f) {
        this->w_pos[2] = -20.0f;
    } else if (this->w_pos[2] < -20.0f) {
        this->w_pos[2] = 20.0f;
    }
    this->acl = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);
    for (auto& fellow : fellow_ships) {
        if (this != &fellow && glm::distance(fellow.w_pos, this->w_pos) < 5.0f) {
            if (glm::dot(glm::cross(glm::vec3(this->vel), glm::vec3(fellow.w_pos - this->w_pos)), this->up) > 0.0f) {
                this->acl = glm::vec4(this->vel[1] * -1.0f, 0.0f, this->vel[0], 0.0f) / 100.0f;
            } else {
                this->acl = glm::vec4(this->vel[1], 0.0f, this->vel[0] * -1.0f, 0.0f) / 100.0f;
            }
            break;
        }
    }
}
