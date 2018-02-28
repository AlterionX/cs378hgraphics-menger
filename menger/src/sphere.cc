#include "sphere.h"

#include <glm/gtx/string_cast.hpp>
#include <iostream>

#define PI 3.14159265359

// {upper, lower}
std::vector<glm::uvec2> base_shifts[2] = {std::vector<glm::uvec2>({
    glm::uvec2(0, 0),
    glm::uvec2(1, 0),
    glm::uvec2(0, 1)
}), std::vector<glm::uvec2>({
    glm::uvec2(1, 1),
    glm::uvec2(0, 1),
    glm::uvec2(1, 0)
})};

void sphere::create_sphere(float radius, int spoke_cnt, int tier_cnt, std::vector<glm::vec4>& obj_vertices, std::vector<glm::uvec3>& obj_faces) {
    for (int hu = 1; hu < tier_cnt - 1; ++hu) { // angle about z-axis
        double pitch_angle = hu * PI / (tier_cnt - 1) + PI / 2;
        for (int tu = 0; tu < spoke_cnt; ++tu) { // angle about y-axis
            double yaw_angle = tu * 2 * PI / spoke_cnt;
            if (hu != tier_cnt - 2) {
                // create the two tris
                auto upper_face = glm::uvec3(
                    (hu - 1) * spoke_cnt + tu,
                    hu * spoke_cnt + tu,
                    (hu - 1) * spoke_cnt + tu + 1
                );
                auto bottom_face = glm::uvec3(
                    hu * spoke_cnt + tu + 1,
                    (hu - 1) * spoke_cnt + tu + 1,
                    hu * spoke_cnt + tu
                );
                obj_faces.push_back(upper_face);
                std::cout << glm::to_string(obj_faces.back()) << std::endl;
                obj_faces.push_back(bottom_face);
                std::cout << glm::to_string(obj_faces.back()) << std::endl;
            }
            obj_vertices.push_back(glm::vec4(
                radius * glm::cos(pitch_angle) * glm::sin(yaw_angle),
                radius * glm::sin(pitch_angle),
                radius * glm::cos(pitch_angle) * glm::cos(yaw_angle),
                1.0f
            ));
            std::cout << glm::to_string(obj_vertices.back()) << std::endl;
        }
    }
    // add caps
    obj_vertices.push_back(glm::vec4(0.0f, radius, 0.0f, 1.0f));
    std::cout << glm::to_string(obj_vertices.back()) << std::endl;
    obj_vertices.push_back(glm::vec4(0.0f, -radius, 0.0f, 1.0f));
    std::cout << glm::to_string(obj_vertices.back()) << std::endl;
    // faces for those caps
    for (int tu = 0; tu < spoke_cnt; ++tu) {
        auto topface = glm::uvec3(
            obj_vertices.size() - 2,
            tu, (tu + 1) % spoke_cnt
        );
        auto botface = glm::uvec3(
            tu + (tier_cnt - 3) * spoke_cnt,
            (tu + 1) % spoke_cnt + (tier_cnt - 3) * spoke_cnt,
            obj_vertices.size() - 1
        );
        obj_faces.push_back(topface);
        std::cout << glm::to_string(obj_faces.back()) << std::endl;
        obj_faces.push_back(botface);
        std::cout << glm::to_string(obj_faces.back()) << std::endl;
    }
}
