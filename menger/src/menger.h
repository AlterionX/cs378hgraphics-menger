#ifndef MENGER_H
#define MENGER_H

#include <glm/glm.hpp>
#include <vector>

class Menger {
public:
	Menger();
	~Menger();
	void set_nesting_level(int);
	bool is_dirty() const;
	void set_clean();
	void generate_geometry(std::vector<glm::vec4>& obj_vertices,
		std::vector<glm::uvec3>& obj_faces) const;
private:
    void gen_sub_box(std::vector<glm::vec4>& ov, std::vector<glm::uvec3>& of, unsigned int depth) const;

	int nesting_level_ = 0;
	bool dirty_ = true;
};

#endif
