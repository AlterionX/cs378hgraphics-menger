#ifndef CAMERA_H
#define CAMERA_H

#include <glm/glm.hpp>

class Camera {
public:
    enum class ViewMode { NORMAL = 0, FPS = 1 };

	glm::mat4 get_view_matrix() const;
	// FIXME: add functions to manipulate camera objects.
    void mouse_rot(double dx, double dy);
    void mouse_zoom(double dx, double dy);
    void mouse_strafe(double dx, double dy);

    void move(double dt, int dir);
    void roll(double dt, int dir);
    void pan_x(double dt, int dir);
    void pan_y(double dt, int dir);

    void toggle_mode();

private:
    ViewMode mode = ViewMode::NORMAL;
	float camera_distance_ = 3.0;
	glm::vec3 look_ = glm::vec3(0.0f, 0.0f, -1.0f);
	glm::vec3 up_ = glm::vec3(0.0f, 1.0, 0.0f);
	glm::vec3 eye_ = glm::vec3(0.0f, 0.0f, camera_distance_);
	// Note: you may need additional member variables
};

#endif
