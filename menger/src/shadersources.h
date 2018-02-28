// C++ 11 String Literal
// See http://en.cppreference.com/w/cpp/language/string_literal




/*********************************************************/
/*** vertex **********************************************/

/*** default ***/

const char* menger_vs =
R"zzz(#version 330 core
uniform mat4 view;
uniform vec4 light_position;

in vec4 vertex_position;

out vec4 vs_light_direction;
out vec4 vs_world_position;

void main()
{
	gl_Position = view * vertex_position;
	vs_world_position = vertex_position;
	vs_light_direction = -gl_Position + view * light_position;
})zzz";

/*** no-op (for tessellation) ***/
const char* passthrough_vs =
R"zzz(#version 330 core
uniform mat4 view;

in vec4 vertex_position;

void main()
{
	gl_Position = vertex_position;
})zzz";

const char* floor_vs = passthrough_vs;

const char* ocean_vs = passthrough_vs;

/*********************************************************/
/*** tessellation *****************************************/

/*** triangle ***/

const char* t_ctrl_shader =
R"zzz(#version 430 core
layout (vertices = 3) out;
uniform float tcs_in_deg;
uniform float tcs_out_deg;
void main(void){
    if (gl_InvocationID == 0){
        gl_TessLevelInner[0] = tcs_in_deg;

        gl_TessLevelOuter[0] = tcs_out_deg;
        gl_TessLevelOuter[1] = tcs_out_deg;
        gl_TessLevelOuter[2] = tcs_out_deg;
    }
    gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
}
)zzz";

const char* t_eval_shader =
R"zzz(#version 430 core
layout (triangles) in;
uniform mat4 view;
uniform vec4 light_position;

out vec4 vs_light_direction;
out vec4 vs_world_position;

void main(void){ // TODO: check order of vertices
	vs_world_position = gl_TessCoord.x * gl_in[0].gl_Position
					  + gl_TessCoord.y * gl_in[1].gl_Position
					  + gl_TessCoord.z * gl_in[2].gl_Position;
	gl_Position = view * vs_world_position;
	vs_light_direction = view * light_position - gl_Position;
})zzz";


/*** quadrangle ***/

const char* quad_t_ctrl_shader =
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

const char* ocean_tes =
R"zzz(#version 430 core
layout (quads) in;
uniform mat4 view;
uniform vec4 light_position;
uniform float wave_time;
uniform float tidal_time;

out vec4 vs_light_direction;
out vec4 vs_world_light_direction;
out vec4 vs_world_position;
out vec4 world_tes_norm;

#define M_PI 3.1415926535897932384626433832795
#define TIDAL_LEFT_T 100.0

vec4 lin_interpolate(vec4 A, vec4 B, float t) {
	return A*t + B*(1-t);
}


/* tidal fn + normal */
float moving_gaussian_offset(vec2 pos, vec2 dir, vec2 center, float A, float sigma) {
    vec2 n_c = dir * tidal_time + center;
    float dist = distance(pos, n_c);
    return A * exp(-(dist * dist) / (2 * sigma * sigma)) / (2 * M_PI * sigma * sigma);
}
float tidal_offset(float x, float y) {
    vec2 dir =      vec2(1, 0);
    vec2 center =   vec2(0, 0);
    float A =       40.0;
    float sigma =   1.0;
    return moving_gaussian_offset(vec2(x, y), dir, center, A, sigma);
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

/* wave fn + normal */
float single_wave_offset(vec2 wave_dir, vec2 pos, float A, float freq, float phase, float k) {
    return 2 * A * pow((sin(dot(wave_dir, pos) * freq + wave_time * phase) + 1) / 2, k);
}

float wave_offset(float x, float y) {
    vec2 wave_dir = vec2(1, 1);
    float A = 0.5;
    float freq = 0.5;
    float phase = 0.01 * freq;
    float shift = 5;
    return single_wave_offset(wave_dir, vec2(x, y), A, freq, phase, shift);
}
vec3 single_wave_normal(vec2 wave_dir, vec2 pos, float A, float freq, float phase, float k) {
    float dx = k * wave_dir[0] * freq * A * pow((sin(dot(wave_dir, pos) * freq + wave_time * phase) + 1) / 2, k - 1) * cos(dot(wave_dir, pos) * freq + wave_time * phase);
    float dy = k * wave_dir[1] * freq * A * pow((sin(dot(wave_dir, pos) * freq + wave_time * phase) + 1) / 2, k - 1) * cos(dot(wave_dir, pos) * freq + wave_time * phase);
    return vec3(-dx, 1, -dy);
}
vec4 wave_normal(float x, float y) {
    vec2 wave_dir = vec2(1, 1);
    float A = 0.5;
    float freq = 0.5;
    float phase = 0.01 * freq;
    float shift = 5;
    return vec4(single_wave_normal(wave_dir, vec2(x, y), A, freq, phase, shift), 0.0);
}

void main(void) { // TODO: check order of vertices
	vec4 p1 = lin_interpolate(gl_in[0].gl_Position, gl_in[3].gl_Position, gl_TessCoord.x);
	vec4 p2 = lin_interpolate(gl_in[1].gl_Position, gl_in[2].gl_Position, gl_TessCoord.x);
	vs_world_position = lin_interpolate(p1, p2, gl_TessCoord.y);

    vs_world_position += vec4(0.0, wave_offset(vs_world_position[0], vs_world_position[2]), 0.0, 0.0);
    if(tidal_time < TIDAL_LEFT_T)
        vs_world_position += vec4(0.0, tidal_offset(vs_world_position[0], vs_world_position[2]), 0.0, 0.0);

	gl_Position = view * vs_world_position;
    vs_light_direction = view * light_position - gl_Position;
    vs_world_light_direction = light_position - vs_world_position;

    world_tes_norm = wave_normal(vs_world_position[0], vs_world_position[2]);
    if(tidal_time < TIDAL_LEFT_T)
        world_tes_norm += tidal_normal(vs_world_position[0], vs_world_position[2]);
    world_tes_norm = normalize(world_tes_norm);
}
)zzz";




/*********************************************************/
/*** geometry ********************************************/

const char* menger_gs =
R"zzz(#version 330 core
layout (triangles) in;
layout (triangle_strip, max_vertices = 3) out;
uniform mat4 projection;

in vec4 vs_light_direction[];
in vec4 vs_world_position[];

flat out vec4 normal;
flat out vec4 world_normal;
out vec4 light_direction;
out vec4 world_position;
out vec4 position;
out vec3 edge_dist;

void main()
{
	vec3 wp0 = vec3(vs_world_position[0]);
    vec3 wp1 = vec3(vs_world_position[1]);
    vec3 wp2 = vec3(vs_world_position[2]);
    world_normal = vec4(normalize(cross(wp1 - wp0, wp2 - wp0)), 1.0);

    vec3 p0 = gl_in[0].gl_Position.xyz;
    vec3 p1 = gl_in[1].gl_Position.xyz;
    vec3 p2 = gl_in[2].gl_Position.xyz;
    normal = vec4(normalize(cross(p1 - p0, p2 - p0)), 1.0);

    vec3 dist_scale = vec3(
        length(cross(p1 - p0, p2 - p0)/length(p2 - p1)),
        length(cross(p0 - p1, p2 - p1)/length(p2 - p0)),
        length(cross(p1 - p2, p0 - p2)/length(p1 - p0))
    );

    int n;
    for (n = 0; n < gl_in.length(); n++) {;
		light_direction = vs_light_direction[n];
		gl_Position = projection * gl_in[n].gl_Position;
		world_position = vs_world_position[n];;
        position = gl_in[n].gl_Position;
        edge_dist = vec3(float(n == 0), float(n == 1), float(n == 2)) * dist_scale;
		EmitVertex();
	}
    EndPrimitive();
}
)zzz";

const char* ocean_gs =
R"zzz(#version 330 core
layout (triangles) in;
layout (triangle_strip, max_vertices = 3) out;
uniform mat4 projection;

in vec4 vs_light_direction[];
in vec4 vs_world_light_direction[];
in vec4 vs_world_position[];
in vec4 world_tes_norm[];

out vec4 normal;
flat out vec4 world_normal;
out vec4 light_direction;
out vec4 world_position;
out vec3 edge_dist;
out vec4 position_camspace;

void main() {
	vec3 wp0 = vec3(vs_world_position[0]);
    vec3 wp1 = vec3(vs_world_position[1]);
    vec3 wp2 = vec3(vs_world_position[2]);
    world_normal = vec4(normalize(cross(wp1 - wp0, wp2 - wp0)), 1.0);

    vec3 p0 = gl_in[0].gl_Position.xyz;
    vec3 p1 = gl_in[1].gl_Position.xyz;
    vec3 p2 = gl_in[2].gl_Position.xyz;
    // normal = vec4(normalize(cross(p1 - p0, p2 - p0)), 1.0);

    vec3 dist_scale = vec3(
        length(cross(p1 - p0, p2 - p0)/length(p2 - p1)),
        length(cross(p0 - p1, p2 - p1)/length(p2 - p0)),
        length(cross(p1 - p2, p0 - p2)/length(p1 - p0))
    );

    int n;
    for (n = 0; n < gl_in.length(); n++) {
        // light_direction = vs_light_direction[n];
        light_direction = vs_world_light_direction[n];

        world_position = vs_world_position[n];
        position_camspace = gl_in[n].gl_Position;
		gl_Position = projection * position_camspace;
        edge_dist = vec3(float(n == 0), float(n == 1), float(n == 2)) * dist_scale;
        normal = world_tes_norm[n];
        //normal = world_normal;
		EmitVertex();
	}
    EndPrimitive();
})zzz";




/*********************************************************/
/*** fragment ********************************************/

const char* menger_fs =
R"zzz(#version 330 core
uniform bool render_wireframe;
flat in vec4 normal;
flat in vec4 world_normal;
in vec4 light_direction;
in vec3 edge_dist;
out vec4 fragment_color;
void main() {
    /*if (render_wireframe && (edge_dist[0] < 0.02 || edge_dist[1] < 0.02 || edge_dist[2] < 0.02)) {
        fragment_color = vec4(0.0, 1.0, 0.0, 1.0);
    } else*/ {
        vec4 color = clamp(world_normal * world_normal, 0.0, 1.0);
        float dot_nl = dot(normalize(light_direction), normalize(normal));
        dot_nl = clamp(dot_nl, 0.0, 1.0);
        fragment_color = clamp(dot_nl * color, 0.0, 1.0);
    }
})zzz";

const char* floor_fs =
R"zzz(#version 330 core
uniform bool render_wireframe;
flat in vec4 normal;
in vec4 light_direction;
in vec4 world_position;
in vec3 edge_dist;

out vec4 fragment_color;

void main() {
    if (render_wireframe && (edge_dist[0] < 0.02 || edge_dist[1] < 0.02 || edge_dist[2] < 0.02)) {
        fragment_color = vec4(0.0, 1.0, 0.0, 1.0);
    } else {
        vec4 color = vec4(1.0, 1.0, 1.0, 1.0);
        if (mod(floor(world_position[0])+floor(world_position[2]), 2) == 0) {
            color *= 0;
        }
        float dot_nl = dot(normalize(light_direction), normalize(normal));
        dot_nl = clamp(dot_nl, 0.0, 1.0);
        fragment_color = clamp(dot_nl * color, 0.0, 1.0);
    }
})zzz";

const char* ocean_fs =
R"zzz(#version 330 core
uniform bool render_wireframe;
uniform mat4 view;
uniform vec4 light_position;

in vec4 normal;
in vec4 light_direction;
in vec4 position_camspace;
in vec4 world_position;
in vec3 edge_dist;

out vec4 fragment_color;

void main() {
    if (render_wireframe && (edge_dist[0] < 0.02 || edge_dist[1] < 0.02 || edge_dist[2] < 0.02)) {
       fragment_color = vec4(0.0, 1.0, 0.0, 1.0);
    } else {
        vec3 ka = vec3(0.01, 0.01, 0.1);
        vec3 kd = vec3(0.5, 0.5, 1.0);
        vec3 ks = vec3(1.0, 1.0, 1.0);
        float alpha = 50;

        vec3 ld = normalize((world_position - light_position).xyz);
        vec3 ed = vec3(inverse(view) * vec4(normalize(position_camspace.xyz), 0.0));
        vec3 surf_norm = -normalize(normal.xyz);

        vec3 ambient = ka;
        vec3 diffuse = kd * max(dot(ld, surf_norm), 0.0);
        vec3 specular = ks * pow(
            max(-dot(
                vec4(ed, 0.0),
                reflect(vec4(ld, 0.0), vec4(surf_norm, 0.0))
            ), 0.0),
            alpha
        );

        //distance attenuate?
        fragment_color = vec4(vec3(clamp(abs(normal)[2], 0.0, 1.0)), 1.0);
        fragment_color = vec4(vec3(clamp(normalize(view[3] - world_position), 0.0, 1.0)), 1.0);
        fragment_color = vec4(vec3(clamp(ed, 0.0, 1.0)), 1.0);
        fragment_color = vec4(ld, 1.0);
        fragment_color = vec4(vec3(reflect(vec4(ld, 0.0), vec4(surf_norm, 0.0))), 1.0);
        fragment_color = vec4(diffuse, 1.0);
        fragment_color = vec4(specular, 1.0);
        fragment_color = vec4(ambient + vec3(1.0, 1.0, 1.0) * (diffuse + specular), 1.0);
    }
})zzz";
