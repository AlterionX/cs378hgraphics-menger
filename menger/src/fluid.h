#ifndef __FLUID_H__
#define __FLUID_H__

#include <glm/glm.hpp>
#include <vector>

namespace fluid {
    struct wave_params {
        float a;
        float l;
        float s;
        float k;
        glm::vec2 dir;

        double life = 0;
        double time = 0;

        float wavel(void) const;
        float phase(void) const;
    };

    struct gaussian_params {
        glm::vec2 dir;
        glm::vec2 center;
        float A;
        float sigma;
        double start;
    };

    struct wave_packet {
        double start;
        glm::vec2 center;
    };

    struct ocean_surf_params {
        std::vector<wave_params> wpars;
        gaussian_params gp;
        std::vector<wave_packet> wpacks;

        unsigned int storminess = 0;

        void elapse_time(double elapsed);
    };

    glm::vec4 simulate_offset(double t, glm::vec2& pos, ocean_surf_params& ospars);
    glm::vec4 simulate_normal(double t, glm::vec2& pos, ocean_surf_params& ospars);
    wave_params generate_wave(int storminess, int count);
}

#endif
