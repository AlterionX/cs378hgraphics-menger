#ifndef __FLUID_H__
#define __FLUID_H__

#include <glm/glm.hpp>
#include <vector>

namespace fluid {
    struct wave_params {
        float A;
        glm::vec2 dir;
        float freq;
        float phase;
        float k;
    };

    struct gaussian_params {
        glm::vec2 dir;
        glm::vec2 center;
        float A;
        float sigma;
    };

    struct wave_packet {
        double start;
        glm::vec2 center;
    };

    struct ocean_surf_params {
        std::vector<wave_params> wpars;
        gaussian_params gp;
        std::vector<wave_packet> wpacks;
    };

    glm::vec4 simulate_offset(double t, glm::vec2& pos, ocean_surf_params& ospars);
}

#endif
