#include "camera.h"
#include "glm/glm.hpp"
#include "glm/gtx/transform.hpp"
#include <glm/gtx/string_cast.hpp>

#include <iostream>

namespace {
    float pan_speed = 0.1f;
    float roll_speed = 0.1f;
    float rotation_speed = 0.05f;
    float zoom_speed = 0.1f;
};

void Camera::change_mode(void) {
    mode =(ViewMode)((int)mode ^ 1);
}

// FIXME: Calculate the view matrix
glm::mat4 Camera::get_view_matrix() const {
    auto forw = -look_;
    auto right = glm::normalize(glm::cross(look_, up_));
    auto true_up = glm::normalize(glm::cross(right, look_));

    auto trans = glm::translate(-eye_);
    auto cob_mat = glm::mat4(glm::transpose(glm::mat3(right, true_up, forw)));
    return cob_mat * trans;
}

void Camera::mouse_rot(double dx, double dy) {
    auto forw = look_;
    auto up = up_;
    auto right = glm::cross(forw, up_);

    bool is_fps = this->mode == (ViewMode::FPS);
    auto center = is_fps ? eye_ : (eye_ + camera_distance_ * forw);

    float yaw_ang = glm::atan(dx / camera_distance_) * rotation_speed;
    float pitch_ang = glm::atan(-dy / camera_distance_) * rotation_speed;

    auto yaw_rot_mat = glm::translate(-center);
    yaw_rot_mat = glm::rotate(yaw_rot_mat, yaw_ang, up_);
    yaw_rot_mat = glm::translate(yaw_rot_mat, center);
    if(!is_fps) eye_ = glm::vec3(yaw_rot_mat * glm::vec4(eye_, 1.0f));
    look_ = glm::vec3(yaw_rot_mat * glm::vec4(look_, 0.0f));

    auto pitch_rot_mat = glm::translate(-center);
    pitch_rot_mat = glm::rotate(pitch_rot_mat, pitch_ang, right);
    pitch_rot_mat = glm::translate(pitch_rot_mat, center);
    if(!is_fps) eye_ = glm::vec3(pitch_rot_mat * glm::vec4(eye_, 1.0f));

    up_ = glm::vec3(pitch_rot_mat * glm::vec4(up_, 0.0f));
    look_ = glm::vec3(pitch_rot_mat * glm::vec4(look_, 0.0f));
    look_ = glm::normalize(look_);
    up_ = glm::normalize(up_);
}
void Camera::mouse_zoom(double dx, double dy) { // ignore x
    float diff = dy * zoom_speed;
    if (camera_distance_ + diff > 0.1) {
        camera_distance_ += diff;
    } else {
        camera_distance_ = 0.1;
}
}
void Camera::mouse_strafe(double dx, double dy) {
    //do nothing, for now.
}

void Camera::move(double dt, int dir) {
    float amount = zoom_speed * dt * dir;
    eye_ += amount * look_;
}
void Camera::roll(double dt, int dir) {
    float roll_ang = roll_speed * dt * dir;
    auto roll_rot_mat = glm::rotate(roll_ang, look_);
    up_ = glm::normalize(glm::vec3(roll_rot_mat * glm::vec4(up_, 0.0f)));
}
void Camera::pan_x(double dt, int dir) {
    float amount = pan_speed * dt * dir;
    eye_ += glm::normalize(glm::cross(look_, up_)) * amount;
}
void Camera::pan_y(double dt, int dir) {
    float amount = pan_speed * dt * dir;
    eye_ += up_ * amount;
}

float Camera::get_fov(float deg) {
    return glm::clamp(deg / 180.0f * camera_distance_ / 3.0f * 3.14159265359f, 0.0f, 3.1413f);
}

void Camera::reset(void) {
    mode = ViewMode::NORMAL;
	camera_distance_ = 3.0f;
	look_ = glm::normalize(glm::vec3(0.0f, -1.0f, -1.0f));
	up_ = glm::normalize(glm::vec3(0.0f, 1.0f, -1.0f));
	eye_ = glm::vec3(0.0f, 10.0f, 10.0f);
}
