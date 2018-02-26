#include "ocean.h"

#include <iostream>

void Ocean::generate_geometry(std::vector<glm::vec4>& obj_vertices, std::vector<glm::uvec4>& obj_faces) {
    is_dirty = false;
    obj_vertices.clear();
    obj_faces.clear();
    this->generate_bases(obj_vertices, obj_faces);
    for (auto& vert : obj_vertices) {
        offset_vert(vert);
    }
}

void Ocean::reset(void) {
    is_dirty = true;
}

void Ocean::offset_vert(glm::vec4& vert) {
    vert *= glm::vec4(40.0f, 1.0f, 40.0f, 1.0f);
    vert -= glm::vec4(20.0f, 0.0f, 20.0f, 0.0f);
}

bool Ocean::dirty(void) const {
    return is_dirty;
}

void Ocean::generate_bases(std::vector<glm::vec4>& obj_vertices, std::vector<glm::uvec4>& obj_faces) {
    for (int i = 0; i < Ocean::row + 1; ++i) {
        for (int j = 0; j < Ocean::col + 1; ++j) {
            auto loc = glm::vec4(((double) i) / Ocean::row, -2.0f, ((double)j) / Ocean::col, 1.0f);
            obj_vertices.push_back(loc);
            if (i != Ocean::row && j != Ocean::col) {
                auto small_loc = glm::uvec2(i, j);
                glm::uvec4 face;
                for (int v = 0; v < 4; ++v) {
                    auto vert_num = (small_loc + Ocean::base_shifts[v]);
                    vert_num *= Ocean::shift;
                    face[v] = vert_num[0] + vert_num[1];
                }
                obj_faces.push_back(face);
            }
        }
    }
}

glm::uvec2 Ocean::shift = glm::uvec2(Ocean::row + 1, 1);
std::vector<glm::uvec2> Ocean::base_shifts = std::vector<glm::uvec2>({
    glm::uvec2(1, 1),
    glm::uvec2(0, 1),
    glm::uvec2(0, 0),
    glm::uvec2(1, 0)
});
