#version 450

#include "lib/atmosphere.glsl"

// -------------------------------- I/O --------------------------------

layout(set = 0, binding = 0) uniform Falloff {
    float density_falloff;
};

layout(location = 0) in vec2 in_uv;

layout(location = 0) out float out_color;


const uint sample_count = 64;

const vec3 center = vec3(0.0, 0.0, 0.0);
const float planet_radius = 1.0;
const float atmosphere_height = 1.0;
const float radius = planet_radius + atmosphere_height;


// -------------------------------- HELPERS --------------------------------
float normalized_altitude(float altitude) {
    return altitude / (radius - planet_radius);
}

float normalized_altitude(vec3 pos) {
    const float altitude = length(center - pos) - planet_radius;
    return normalized_altitude(altitude);
}

float atmosphere_density(float norm_alt) {
    return exp(-norm_alt * density_falloff) * (1.0 - norm_alt);
}

float atmosphere_density(vec3 pos) {
    return atmosphere_density(normalized_altitude(pos));
}

float optical_depth(vec3 orig, vec3 dir, float len) {
    const float step_size = len / (sample_count - 1);
    const vec3 step = dir * step_size;

    float depth = 0.0;
    vec3 pos = orig;
    for(uint i = 0; i != sample_count; ++i) {
        const float density = atmosphere_density(pos);
        depth += density * step_size;
        pos += step;
    }

    return depth;
}


// -------------------------------- MAIN --------------------------------

void main() {
    const float cos_t = in_uv.x * 2.0 - 1.0;
    const float sin_t = sqrt(1.0 - sqr(cos_t));
    const vec3 dir = vec3(cos_t, 0.0,sin_t);

    const float norm_alt = in_uv.y;
    const float altitude = planet_radius + norm_alt;
    const vec3 orig = vec3(0.0, 0.0, altitude);

    const float depth = optical_depth(orig, dir, 2.0 * radius);

    out_color = depth;
}
