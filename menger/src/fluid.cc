#include "fluid.h"

#include <glm/gtc/constants.hpp>
#include <glm/gtx/string_cast.hpp>
#include <iostream>

#include <cstdlib>

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
    float cal = wpars.time / wpars.life;
    return 2 * wpars.a * ((1 - cal) * cal) * glm::pow((glm::sin(glm::dot(wpars.dir, pos) * wpars.wavel() + t * wpars.phase()) + 1) / 2, wpars.k);
}
glm::vec4 single_wave_normal(double t, glm::vec2 pos, fluid::wave_params& wpars) {
    float cal = wpars.time / wpars.life;

    float basis = wpars.k * wpars.wavel() * wpars.a * ((1 - cal) * cal) * glm::pow(
        (glm::sin(glm::dot(wpars.dir, pos) * wpars.wavel() + t * wpars.phase()) + 1) / 2,
        wpars.k - 1
    ) * glm::cos(glm::dot(wpars.dir, pos) * wpars.wavel() + t * wpars.phase());
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
    if (tidal_time < 100) {
        offset += tidal_offset(tidal_time, pos, ospars.gp);
    }
    return offset;
}
glm::vec4 fluid::simulate_normal(double t, glm::vec2& pos, fluid::ocean_surf_params& ospars) {
    auto normal = wave_normal(t, pos, ospars.wpars);
    double tidal_time = t - ospars.gp.start;
    if (tidal_time < 100) {
        normal += tidal_normal(tidal_time, pos, ospars.gp);
    }
    return normal;
}

float fluid::wave_params::wavel(void) const {
    return 2 / l;
}
float fluid::wave_params::phase(void) const {
    return 2 / l * s;
}

fluid::wave_params fluid::generate_wave(int storminess, int count) {
    int r = rand();
    auto ret = fluid::wave_params {
        // amp
        r % 10560 / 7500.0f * (storminess/2 + 1),
        // length
        float(r % 5 + 2) / (storminess/2 + 1) / float(1.0 / (count + 1)),
        // speed
        r % 500 / 100000.0f * (storminess/2 + 1) * float(1.0 / (count + 1)),
        // steepness
        float(r % 5),
        // direction
        glm::vec2(r % 5503 / 5503.0f, r % 10073 / 10073.0f) * glm::pow(-1.0f, (r % 2) + 1.0f),
        // lifetime
        double(((r % 50) + 20) * 100000),
        0
    };
    return ret;
}

void fluid::ocean_surf_params::elapse_time(double elapsed) {
    for (size_t i = 0; i < this->wpars.size(); ++i) {
        this->wpars[i].time += elapsed / 1000;
        if (this->wpars[i].life < this->wpars[i].time) {
            if (i < 3 + this->storminess) {
                this->wpars[i] = generate_wave(this->storminess, i);
            }
        }
    }
    while (this->wpars.size() < 3 + this->storminess /*|| max_amp < target_amp*/) {
        this->wpars.push_back(generate_wave(this->storminess, this->wpars.size() + 1));
    }
}
