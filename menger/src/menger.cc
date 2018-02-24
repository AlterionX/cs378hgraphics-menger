#include "menger.h"

#include <iostream>
#include <glm/gtx/string_cast.hpp>

namespace {
	const int kMinLevel = 0;
	const int kMaxLevel = 4;
};

static const std::vector<glm::vec4> bc_ps = std::vector<glm::vec4>({
    glm::vec4(0.5f, 0.5f, 0.5f, 1.0f), // top, right, out
    glm::vec4(0.5f, -0.5f, 0.5f, 1.0f), // bottom, ro
    glm::vec4(-0.5f, -0.5f, 0.5f, 1.0f), // b, left, o
    glm::vec4(-0.5f, 0.5f, 0.5f, 1.0f), // tlo
    glm::vec4(-0.5f, 0.5f, -0.5f, 1.0f), // tl, in
    glm::vec4(-0.5f, -0.5f, -0.5f, 1.0f), // bli
    glm::vec4(0.5f, -0.5f, -0.5f, 1.0f), // bri
    glm::vec4(0.5f, 0.5f, -0.5f, 1.0f) // tri
});
static const std::vector<glm::uvec3> bc_fs = std::vector<glm::uvec3>({
    glm::uvec3(0, 2, 1), // front bottom
    glm::uvec3(0, 3, 2), // f top
    glm::uvec3(3, 4, 5), // left t
    glm::uvec3(3, 5, 2), // lb
    glm::uvec3(4, 7, 6), // back t
    glm::uvec3(4, 6, 5), // bb
    glm::uvec3(7, 1, 6), // right t
    glm::uvec3(7, 0, 1), // rb
    glm::uvec3(7, 4, 0), // top t
    glm::uvec3(3, 0, 4), // tb
    glm::uvec3(1, 2, 6), // bottom t
    glm::uvec3(5, 6, 2) // bb
});
static constexpr float scale = 1.0f / 3.0f;
// Center of the top right out sub box
static const glm::vec4 base = glm::vec4(0.5f - scale / 2, 0.5f - scale / 2, 0.5f - scale / 2, 1.0f);
static const std::vector<glm::vec3> shifts = std::vector<glm::vec3>({
    glm::vec3(0, 0, 0),
    glm::vec3(1, 0, 0),
    glm::vec3(2, 0, 0),
    glm::vec3(0, 1, 0),
    glm::vec3(2, 1, 0),
    glm::vec3(0, 2, 0),
    glm::vec3(1, 2, 0),
    glm::vec3(2, 2, 0),
    glm::vec3(0, 0, 1),
    glm::vec3(2, 0, 1),
    glm::vec3(0, 2, 1),
    glm::vec3(2, 2, 1),
    glm::vec3(0, 0, 2),
    glm::vec3(1, 0, 2),
    glm::vec3(2, 0, 2),
    glm::vec3(0, 1, 2),
    glm::vec3(2, 1, 2),
    glm::vec3(0, 2, 2),
    glm::vec3(1, 2, 2),
    glm::vec3(2, 2, 2)
});

Menger::Menger() {
	// Add additional initialization if you like
}

Menger::~Menger() {}

void Menger::set_nesting_level(int level) {
    if (nesting_level_ != level + 1) {
    	nesting_level_ = level + 1;
    	dirty_ = true;
    }
}

bool Menger::is_dirty() const {
	return dirty_;
}

void Menger::set_clean() {
	dirty_ = false;
}

// FIXME generate Menger sponge geometry
void Menger::generate_geometry(
	std::vector<glm::vec4>& obj_vertices,
	std::vector<glm::uvec3>& obj_faces
) const {
    obj_vertices.clear();
    obj_faces.clear();
    gen_sub_box(obj_vertices, obj_faces, nesting_level_);
}

void Menger::gen_sub_box(std::vector<glm::vec4>& ov, std::vector<glm::uvec3>& of, unsigned int depth) const {
    if (depth == 0) {
        ov.insert(ov.end(), bc_ps.begin(), bc_ps.end());
        of.insert(of.end(), bc_fs.begin(), bc_fs.end());
    } else {
        // Generate submatrix
        std::vector<glm::vec4> sub_v;
        std::vector<glm::uvec3> sub_f;
        gen_sub_box(sub_v, sub_f, depth - 1);
        // Scale down to size
        for (auto& it : sub_v) {
            it *= glm::vec4(glm::vec3(scale), 1.0f);
        }
        // Generate for each shift the proper sub box
        for (size_t i = 0; i < shifts.size(); ++i) {
            auto shift = shifts[i];
            // Based on the base center point, create each of the points based on the model
            for (auto it : sub_v) {
                ov.push_back(base - glm::vec4(shift * scale, 1.0f) + it);
            }
            // Insert the given vertices with the proper start
            for (auto face_num : sub_f) {
                of.push_back(face_num + glm::uvec3(i * sub_v.size()));
            }
        }
    }
}
