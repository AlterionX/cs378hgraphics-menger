// vs -> tcs -> tes -> gs -> fs
// world -> world -> world -> camera -> projection

/*********************************************************/
/*** vertex **********************************************/

/*** no-op (for tessellation) ***/
const char* passthrough_vs =
R"zzz(#version 430 core
in vec4 w_pos;

void main() {
	gl_Position = w_pos;
})zzz";

/*** convert to view basis ***/
const char* cob_vs =
R"zzz(#version 430 core
uniform mat4 view;
uniform vec4 w_lpos;

in vec4 w_pos;

out vec4 v_v_from_ldir;
out vec4 v_w_pos;

void main() {
	v_w_pos = w_pos;
    gl_Position = view * w_pos;
    v_v_from_ldir = view * (w_pos - w_lpos);
})zzz";

/*** convert to view basis and shift by model ***/
const char* cob_model_vs =
R"zzz(#version 430 core
uniform mat4 view;
uniform mat4 model;
uniform vec4 w_lpos;

in vec4 w_pos;

out vec4 v_v_from_ldir;
out vec4 v_w_pos;

void main() {
	v_w_pos = model * w_pos;
    gl_Position = view * v_w_pos;
    v_v_from_ldir = view * (v_w_pos - w_lpos);
})zzz";

/*********************************************************/
/*** tessellation *****************************************/

/*** triangle ***/
const char* simple_tri_tcs =
R"zzz(#version 430 core
layout (vertices = 3) out;

uniform float tcs_in_deg;
uniform float tcs_out_deg;

void main(void) {
    gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
    if (gl_InvocationID == 0){
        gl_TessLevelInner[0] = tcs_in_deg;
        gl_TessLevelOuter[0] = tcs_out_deg;
        gl_TessLevelOuter[1] = tcs_out_deg;
        gl_TessLevelOuter[2] = tcs_out_deg;
    }
})zzz";
const char* simple_tri_tes =
R"zzz(#version 430 core
layout (triangles) in;

uniform mat4 view;
uniform vec4 w_lpos;

out vec4 v_v_from_ldir;
out vec4 v_w_pos;

void main(void){
    v_w_pos = gl_TessCoord.x * gl_in[2].gl_Position
                + gl_TessCoord.y * gl_in[1].gl_Position
                + gl_TessCoord.z * gl_in[0].gl_Position;

	gl_Position = view * v_w_pos;
	v_v_from_ldir = gl_Position - view * w_lpos;
})zzz";

/*** quadrangle ***/
const char* adaptive_quad_tcs =
R"zzz(#version 430 core
layout (vertices = 4) out;
uniform float tcs_in_deg;
uniform float tcs_out_deg;
uniform float tidal_time;

#define TIDAL_LEFT_T 100.0
#define MAX_ADAPTIVE 10
#define TIDAL_DECAY 5

void main(void){
    if (gl_InvocationID == 0){

        float in_deg = tcs_in_deg;
        float out_deg = tcs_out_deg;

        // adaptive tessellation
        if(tidal_time < TIDAL_LEFT_T) {
            float grid_size = distance(gl_in[0].gl_Position.xyz, gl_in[1].gl_Position.xyz);
            vec3 grid_center = (gl_in[0].gl_Position.xyz + gl_in[1].gl_Position.xyz + gl_in[2].gl_Position.xyz + gl_in[3].gl_Position.xyz)/4.0;
            vec2 grid_c = vec2(grid_center[0], grid_center[2]);
            vec2 tidal_c = vec2(1, 0) * tidal_time + vec2(0, 0);
            if(distance(grid_c, tidal_c) / grid_size * TIDAL_DECAY < MAX_ADAPTIVE) {
                in_deg = tcs_in_deg + int(MAX_ADAPTIVE - distance(grid_c, tidal_c) / grid_size * TIDAL_DECAY);
                out_deg = tcs_out_deg + int(MAX_ADAPTIVE - distance(grid_c, tidal_c) / grid_size * TIDAL_DECAY);
            }
        }

        // set tess levels
        gl_TessLevelInner[0] = in_deg;
        gl_TessLevelInner[1] = in_deg;

        gl_TessLevelOuter[0] = out_deg;
        gl_TessLevelOuter[1] = out_deg;
        gl_TessLevelOuter[2] = out_deg;
        gl_TessLevelOuter[3] = out_deg;
    }
    gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
})zzz";
const char* tidal_quad_tes =
R"zzz(#version 430 core
layout (quads) in;
uniform mat4 view;
uniform vec4 w_lpos;
uniform float wave_time;
uniform float wave_type;
uniform float tidal_time;

out vec4 v_v_from_ldir;
out vec4 v_v_norm;

#define M_PI 3.1415926535897932384626433832795
#define TIDAL_LEFT_T 100.0

/* gaussian tidal wave */
float moving_gaussian_offset(vec2 pos, vec2 dir, vec2 center, float A, float sigma) {
    vec2 n_c = dir * tidal_time + center;
    float dist = distance(pos, n_c);
    return A * exp(-(dist * dist) / (2 * sigma * sigma)) / (2 * M_PI * sigma * sigma);
}
vec4 tidal_offset(float x, float y) {
    vec2 dir =      vec2(1, 0);
    vec2 center =   vec2(0, 0);
    float A =       40.0;
    float sigma =   1.0;
    return vec4(0.0f, moving_gaussian_offset(vec2(x, y), dir, center, A, sigma), 0.0f, 0.0f);
}
vec3 moving_gaussian_normal(vec2 pos, vec2 dir, vec2 center, float A, float sigma) {
    vec2 n_c = dir * tidal_time + center;
    float dist = distance(pos, n_c);
    float dx = A * (n_c[0] - pos[0]) * exp(-(dist * dist) / (2 * sigma * sigma)) / (2 * M_PI * pow(sigma, 4));
    float dy = A * (n_c[1] - pos[1]) * exp(-(dist * dist) / (2 * sigma * sigma)) / (2 * M_PI * pow(sigma, 4));
    return vec3(-dx, 1, dy);
}
vec4 tidal_normal(float x, float y) {
    vec2 dir =      vec2(1, 0);
    vec2 center =   vec2(0, 0);
    float A =       40.0;
    float sigma =   1.0;
    return vec4(moving_gaussian_normal(vec2(x, y), dir, center, A, sigma), 0.0);
}

/* regular small waves */
float single_wave_offset(vec2 wave_dir, vec2 pos, float A, float freq, float phase, float k) {
    return 2 * A * pow((sin(dot(wave_dir, pos) * freq + wave_time * phase) + 1) / 2, k);
}

vec4 wave_offset(float x, float y) {
    vec2 wave_dir = vec2(1, 1);
    float A = 0.5;
    float freq = 0.5;
    float phase = 0.01 * freq;
    float shift = 5;
    return vec4(0.0f, single_wave_offset(wave_dir, vec2(x, y), A, freq, phase, shift), 0.0f, 0.0f);
}
vec4 single_wave_normal(vec2 wave_dir, vec2 pos, float A, float freq, float phase, float k) {
    float dx = k * wave_dir[0] * freq * A * pow((sin(dot(wave_dir, pos) * freq + wave_time * phase) + 1) / 2, k - 1) * cos(dot(wave_dir, pos) * freq + wave_time * phase);
    float dy = k * wave_dir[1] * freq * A * pow((sin(dot(wave_dir, pos) * freq + wave_time * phase) + 1) / 2, k - 1) * cos(dot(wave_dir, pos) * freq + wave_time * phase);
    return vec4(-dx, 1, -dy, 0.0);
}
vec4 wave_normal(float x, float y) {
    vec2 wave_dir = vec2(1, 1);
    float A = 0.5;
    float freq = 0.5;
    float phase = 0.01 * freq;
    float shift = 5;
    return single_wave_normal(wave_dir, vec2(x, y), A, freq, phase, shift);
}

void main(void) {
	vec4 p1 = mix(gl_in[0].gl_Position, gl_in[3].gl_Position, gl_TessCoord.x);
	vec4 p2 = mix(gl_in[1].gl_Position, gl_in[2].gl_Position, gl_TessCoord.x);
    vec4 w_pos = mix(p1, p2, gl_TessCoord.y);

    w_pos += wave_offset(w_pos[0], w_pos[2]);
    vec4 w_norm = wave_normal(w_pos[0], w_pos[2]); // assumes original norm is 0

    if(tidal_time < TIDAL_LEFT_T) {
        w_pos += tidal_offset(w_pos[0], w_pos[2]);
        w_norm += tidal_normal(w_pos[0], w_pos[2]);
    }

	gl_Position = view * w_pos;
    v_v_from_ldir = gl_Position - (view * w_lpos);
    v_v_norm = view * w_norm;
})zzz";

/*********************************************************/
/*** geometry ********************************************/

const char* base_gs =
R"zzz(#version 330 core
layout (triangles) in;
layout (triangle_strip, max_vertices = 3) out;

uniform mat4 projection;

in vec4 v_v_from_ldir[];
in vec4 v_w_pos[];

flat out vec4 v_norm;
flat out vec4 w_norm;

out vec4 v_from_ldir;

void main() {
	vec3 wp0 = vec3(v_w_pos[0]);
    vec3 wp1 = vec3(v_w_pos[1]);
    vec3 wp2 = vec3(v_w_pos[2]);
    w_norm = vec4(normalize(cross(wp1 - wp0, wp2 - wp0)), 0.0);

    vec3 p0 = gl_in[0].gl_Position.xyz;
    vec3 p1 = gl_in[1].gl_Position.xyz;
    vec3 p2 = gl_in[2].gl_Position.xyz;
    v_norm = vec4(normalize(cross(p1 - p0, p2 - p0)), 0.0);

    int n;
    for (n = 0; n < gl_in.length(); n++) {
	    v_from_ldir = v_v_from_ldir[n];
		gl_Position = projection * gl_in[n].gl_Position;
		EmitVertex();
	}
    EndPrimitive();
})zzz";

const char* wireframe_gs =
R"zzz(#version 330 core
layout (triangles) in;
layout (triangle_strip, max_vertices = 3) out;
uniform mat4 projection;

in vec4 v_v_from_ldir[];
in vec4 v_w_pos[];

flat out vec4 v_norm;

out vec4 v_from_ldir;
out vec4 w_pos;
out vec3 edge_dist;

void main() {
    vec3 p0 = gl_in[0].gl_Position.xyz;
    vec3 p1 = gl_in[1].gl_Position.xyz;
    vec3 p2 = gl_in[2].gl_Position.xyz;
    v_norm = vec4(normalize(cross(p1 - p0, p2 - p0)), 0.0);
    vec3 dist_scale = vec3(
        length(cross(p1 - p0, p2 - p0) / length(p2 - p1)),
        length(cross(p0 - p1, p2 - p1) / length(p2 - p0)),
        length(cross(p1 - p2, p0 - p2) / length(p1 - p0))
    );

    int n;
    for (n = 0; n < gl_in.length(); n++) {;
        v_from_ldir = v_v_from_ldir[n];
		w_pos = v_w_pos[n];
        edge_dist = vec3(float(n == 0), float(n == 1), float(n == 2)) * dist_scale;
		gl_Position = projection * gl_in[n].gl_Position;
		EmitVertex();
	}
    EndPrimitive();
})zzz";

const char* phong_norm_gs =
R"zzz(#version 330 core
layout (triangles) in;
layout (triangle_strip, max_vertices = 3) out;
uniform mat4 projection;

in vec4 v_v_from_ldir[];
in vec4 v_v_norm[];

out vec4 v_norm;
out vec4 v_from_ldir;
out vec4 v_pos;
out vec3 edge_dist;

void main() {
    vec3 p0 = gl_in[0].gl_Position.xyz;
    vec3 p1 = gl_in[1].gl_Position.xyz;
    vec3 p2 = gl_in[2].gl_Position.xyz;
    vec3 dist_scale = vec3(
        length(cross(p1 - p0, p2 - p0)/length(p2 - p1)),
        length(cross(p0 - p1, p2 - p1)/length(p2 - p0)),
        length(cross(p1 - p2, p0 - p2)/length(p1 - p0))
    );

    int n;
    for (n = 0; n < gl_in.length(); n++) {
        edge_dist = vec3(float(n == 0), float(n == 1), float(n == 2)) * dist_scale;

        v_from_ldir = v_v_from_ldir[n];
        v_norm = v_v_norm[n];
        v_pos = gl_in[n].gl_Position;

		gl_Position = projection * v_pos;
		EmitVertex();
	}
    EndPrimitive();
})zzz";

/*********************************************************/
/*** fragment ********************************************/

/*** frag col = norm ^ 2 w/ light incidence ***/
const char* base_orient_fs =
R"zzz(#version 330 core
uniform mat4 view;

flat in vec4 v_norm;
flat in vec4 w_norm;

in vec4 v_from_ldir;

out vec4 frag_col;

void main() {
    vec4 base_col = clamp(w_norm * w_norm, 0.0, 1.0);
    float intensity = clamp(-dot(v_from_ldir, v_norm), 0.0, 1.0);
    frag_col = clamp(intensity * base_col, 0.0, 1.0);
    frag_col.a = 1.0;
})zzz";

/*** frag col = norm ^ 2 w/ light incidence ***/
const char* wireframe_orient_fs =
R"zzz(#version 330 core
uniform mat4 view;
uniform bool render_wireframe;

flat in vec4 v_norm;
flat in vec4 w_norm;

in vec4 v_from_ldir;
in vec3 edge_dist;

out vec4 frag_col;

void main() {
    if (render_wireframe && min(min(edge_dist[0], edge_dist[1]), edge_dist[2]) < 0.02) {
        frag_col = vec4(0.0, 1.0, 0.0, 1.0);
    } else {
        vec4 base_col = clamp(w_norm * w_norm, 0.0, 1.0);
        float intensity = clamp(-dot(v_from_ldir, v_norm), 0.0, 1.0);
        frag_col = clamp(intensity * base_col, 0.0, 1.0);
        frag_col.a = 1.0;
    }
})zzz";

/*** frag col = checkboard on xz plane w/ light incidence ***/
const char* wireframe_checker_fs =
R"zzz(#version 330 core
uniform bool render_wireframe;

flat in vec4 v_norm;

in vec4 v_from_ldir;
in vec4 w_pos;
in vec3 edge_dist;

out vec4 frag_col;

void main() {
    if (render_wireframe && min(min(edge_dist[0], edge_dist[1]), edge_dist[2]) < 0.02) {
        frag_col = vec4(0.0, 1.0, 0.0, 1.0);
    } else {
        vec4 base_col = vec4(1.0, 1.0, 1.0, 1.0) * float(mod(floor(w_pos[0]) + floor(w_pos[2]), 2) != 0);
        float intensity = clamp(dot(normalize(v_from_ldir), normalize(v_norm)), 0.0, 1.0);
        frag_col = clamp(intensity * base_col, 0.0, 1.0);
    }
})zzz";

/*** frag col = phong + distance atten ***/
const char* wireframe_phong_datten_fs =
R"zzz(#version 330 core
// dist atten
uniform float cterm;
uniform float lterm;
uniform float qterm;
// phong model
uniform vec3 ka;
uniform vec3 kd;
uniform vec3 ks;
uniform float alpha;

uniform bool render_wireframe;
uniform vec4 w_lpos;

in vec4 v_norm;
in vec4 v_from_ldir;
in vec4 v_pos;
in vec3 edge_dist;

out vec4 frag_col;

void main() {
    if (render_wireframe && min(min(edge_dist[0], edge_dist[1]), edge_dist[2]) < 0.02) {
        frag_col = vec4(0.0, 1.0, 0.0, 1.0);
    } else {
        vec3 linten = vec3(1.0, 1.0, 1.0);

        float d = length(vec3(v_pos));
        float d_atten = 1.0 / (cterm + d * lterm + d * d * qterm);

        vec4 l = normalize(v_from_ldir);
        vec4 e = normalize(vec4(vec3(v_pos), 0.0));
        vec4 n = normalize(v_norm);
        vec4 r = -reflect(l, n);

        vec3 amb = ka;
        vec3 dif = kd * max(-dot(l, n), 0.0);
        vec3 spe = ks * pow(max(dot(e, r), 0.0), alpha);

        vec3 base_col = (amb + linten * (dif + spe)) * d_atten;
        frag_col = vec4(base_col, 1.0);
    }
})zzz";

/*** frag col = emission ***/
const char* emissive_fs =
R"zzz(#version 330 core
// dist atten
uniform float cterm;
uniform float lterm;
uniform float qterm;

uniform bool render_wireframe;
in vec3 edge_dist;

out vec4 frag_col;

void main() {
    if (render_wireframe && min(min(edge_dist[0], edge_dist[1]), edge_dist[2]) < 0.02) {
        frag_col = vec4(0.0, 1.0, 0.0, 1.0);
    } else {
        frag_col = vec4(1.0, 1.0, 1.0, 1.0);
    }
})zzz";


/*** Specific pipelines ***/

const char* menger_vs = cob_vs;
const char* menger_tcs = nullptr;
const char* menger_tes = nullptr;
const char* menger_gs = base_gs;
const char* menger_fs = base_orient_fs;

const char* floor_vs = passthrough_vs;
const char* floor_tcs = simple_tri_tcs;
const char* floor_tes = simple_tri_tes;
const char* floor_gs = wireframe_gs;
const char* floor_fs = wireframe_checker_fs;

const char* ocean_vs = passthrough_vs;
const char* ocean_tcs = adaptive_quad_tcs;
const char* ocean_tes = tidal_quad_tes;
const char* ocean_gs = phong_norm_gs;
const char* ocean_fs = wireframe_phong_datten_fs;

const char* light_vs = cob_model_vs;
const char* light_tcs = nullptr;
const char* light_tes = nullptr;
const char* light_gs = wireframe_gs;
const char* light_fs = emissive_fs;

const char* ship_vs = cob_model_vs;
const char* ship_tcs = nullptr;
const char* ship_tes = nullptr;
const char* ship_gs = wireframe_gs;
const char* ship_fs = wireframe_orient_fs;
