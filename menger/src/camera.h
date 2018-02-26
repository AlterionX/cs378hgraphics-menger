#ifndef CAMERA_H
#define CAMERA_H

#include <glm/glm.hpp>

class Camera {
public:
    Camera(void) { reset(); }

	glm::mat4 get_view_matrix() const;
	// FIXME: add functions to manipulate camera objects.
    void mouse_rot(double dx, double dy);
    void mouse_zoom(double dx, double dy);
    void mouse_strafe(double dx, double dy);

    void move(double dt, int dir);
    void roll(double dt, int dir);
    void pan_x(double dt, int dir);
    void pan_y(double dt, int dir);

    void change_mode(void);
    float get_fov(float deg);

    void reset(void);

private:
	enum class ViewMode {FPS, NORMAL};
	ViewMode mode;
	float camera_distance_;
	glm::vec3 look_;
	glm::vec3 up_;
	glm::vec3 eye_;
	// Note: you may need additional member variables
};

#endif
