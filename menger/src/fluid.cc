#include "fluid.h"

#include <glm/gtc/constants.hpp>
#include <glm/gtx/string_cast.hpp>
#include <iostream>

/* gaussian wave */
float moving_gaussian_offset(double t, glm::vec2 pos, fluid::gaussian_params& gp) {
    glm::vec2 n_c = gp.dir * float(t) + gp.center;
    float dist = glm::distance(pos, n_c);
    return gp.A * glm::exp(-(dist * dist) / (2 * gp.sigma * gp.sigma));
}
glm::vec3 moving_gaussian_normal(double t, glm::vec2 pos, fluid::gaussian_params& gp) {
    glm::vec2 n_c = gp.dir * float(t) + gp.center;
    float dist = glm::distance(pos, n_c);
    float base = gp.A * glm::exp(-(dist * dist) / (2 * gp.sigma * gp.sigma)) / (gp.sigma * gp.sigma);
    float dx = base * (n_c[0] - pos[0]);
    float dy = base * (n_c[1] - pos[1]);
    return glm::vec3(-dx, 1, dy);
}

/* gaussian tidal wave */
glm::vec4 tidal_offset(double t,glm::vec2 pos, fluid::gaussian_params& gp) {
    return glm::vec4(0.0f, moving_gaussian_offset(t, pos, gp), 0.0f, 0.0f);
}
glm::vec4 tidal_normal(double t, glm::vec2 pos, fluid::gaussian_params gp) {
    return glm::vec4(moving_gaussian_normal(t, pos, gp), 0.0f);
}

/* regular small wave */
float single_wave_offset(double t, glm::vec2 pos, fluid::wave_params& wpars) {
    return 2 * wpars.a * glm::pow((glm::sin(glm::dot(wpars.dir, pos) * wpars.wavel + t * wpars.phase) + 1) / 2, wpars.k);
}
glm::vec4 single_wave_normal(double t, glm::vec2 pos, fluid::wave_params& wpars) {
    float basis = wpars.k * wpars.wavel * wpars.a * glm::pow(
        (glm::sin(glm::dot(wpars.dir, pos) * wpars.wavel + t * wpars.phase) + 1) / 2,
        wpars.k - 1
    ) * glm::cos(glm::dot(wpars.dir, pos) * wpars.wavel + t * wpars.phase);
    float dx = basis * wpars.dir[0];
    float dy = basis * wpars.dir[1];
    return glm::vec4(-dx, 1, -dy, 0.0);
}

/* regular small waves */
glm::vec4 wave_offset(double t, glm::vec2 pos, std::vector<fluid::wave_params>& wpars_vec) {
    auto offset = glm::vec4();
    for (auto& wave_params: wpars_vec) {
        offset += glm::vec4(0.0f, single_wave_offset(t, pos, wave_params), 0.0f, 0.0f);
    }
    return offset;
}
glm::vec4 wave_normal(double t, glm::vec2 pos, std::vector<fluid::wave_params>& wpars_vec) {
    auto norm = glm::vec4();
    for (auto& wpars: wpars_vec) {
        norm += single_wave_normal(t, pos, wpars);
    }
    return norm;
}

glm::vec4 fluid::simulate_offset(double t, glm::vec2& pos, fluid::ocean_surf_params& ospars) {
    auto offset = wave_offset(t, pos, ospars.wpars);
    double tidal_time = t - ospars.gp.start;
    if (tidal_time < 100000) {
        offset += tidal_offset(tidal_time, pos, ospars.gp);
    }
    return offset;
}
glm::vec4 fluid::simulate_normal(double t, glm::vec2& pos, fluid::ocean_surf_params& ospars) {
    auto normal = wave_normal(t, pos, ospars.wpars);
    double tidal_time = t - ospars.gp.start;
    if (tidal_time < 100000) {
        normal += tidal_normal(tidal_time, pos, ospars.gp);
    }
    return normal;
}


fluid::wave_params::wave_params(float a, float l, float s, float k, glm::vec2 dir) : a(a), dir(dir), wavel(2 / l), phase(2 / l * s), k(k) {}
