#ifndef __OCEAN_H__
#define __OCEAN_H__

#include <glm/glm.hpp>
#include <vector>

class Ocean {
public:
	void generate_geometry(std::vector<glm::vec4>& obj_vertices,
		std::vector<glm::uvec4>& obj_faces);
    void reset(void);
    bool dirty(void) const;
private:
    void generate_bases(std::vector<glm::vec4>& obj_vertices,
		std::vector<glm::uvec4>& obj_faces);
    void offset_vert(glm::vec4& data);
    bool is_dirty = true;

    static constexpr int row = 16;
    static constexpr int col = 16;
    static glm::uvec2 shift;
    static std::vector<glm::uvec2> base_shifts;
};

#endif
