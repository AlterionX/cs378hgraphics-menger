// C++ 11 String Literal
// See http://en.cppreference.com/w/cpp/language/string_literal

/*********************************************************/
/*** vertex **********************************************/

/*** default ***/

const char* menger_vs =
R"zzz(#version 330 core
in vec4 vertex_position;
uniform mat4 view;
uniform vec4 light_position;
out vec4 vs_light_direction;
out vec4 vs_world_position;
void main()
{
	gl_Position = view * vertex_position;
	vs_world_position = vertex_position;
	vs_light_direction = -gl_Position + view * light_position;
}
)zzz";


/*** no-op (for tesselation) ***/

const char* t_vertex_shader =
R"zzz(#version 330 core
in vec4 vertex_position;
uniform mat4 view;
void main()
{
	gl_Position = vertex_position;
}
)zzz";

const char* floor_vs = t_vertex_shader;

const char* ocean_vs =
R"zzz(#version 330 core
in vec4 vertex_position;
uniform mat4 view;
void main()
{
	gl_Position = vertex_position;
})zzz";

/*********************************************************/
/*** tesselation *****************************************/

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
	vs_light_direction = -gl_Position + view * light_position;
})zzz";


/*** quadrangle ***/

const char* quad_t_ctrl_shader =
R"zzz(#version 430 core
layout (vertices = 4) out;
uniform float tcs_in_deg;
uniform float tcs_out_deg;
void main(void){
    if (gl_InvocationID == 0){
        gl_TessLevelInner[0] = tcs_in_deg;
        gl_TessLevelInner[1] = tcs_in_deg;

        gl_TessLevelOuter[0] = tcs_out_deg;
        gl_TessLevelOuter[1] = tcs_out_deg;
        gl_TessLevelOuter[2] = tcs_out_deg;
        gl_TessLevelOuter[3] = tcs_out_deg;
    }
    gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
}
)zzz";

const char* quad_t_eval_shader =
R"zzz(#version 430 core
layout (quads) in;
uniform mat4 view;
uniform vec4 light_position;
uniform float wave_time;

out vec4 vs_light_direction;
out vec4 vs_world_position;

vec4 lin_interpolate(vec4 A, vec4 B, float t) {
	return A*t + B*(1-t);
}
float single_wave_offset(vec2 wave_dir, vec2 pos, float A, float freq, float phase) {
    return A * sin(dot(wave_dir, pos) * freq + wave_time / 1000 * phase);
}
float wave_offset(float x, float y) {
    vec2 wave_dir = vec2(1, 1);
    float A = 0.5;
    float freq = 0.5;
    float phase = 0.01 * freq;
    return single_wave_offset(wave_dir, vec2(x, y), A, freq, phase);
}
void main(void){ // TODO: check order of vertices
	vec4 p1 = lin_interpolate(gl_in[0].gl_Position, gl_in[3].gl_Position, gl_TessCoord.x);
	vec4 p2 = lin_interpolate(gl_in[1].gl_Position, gl_in[2].gl_Position, gl_TessCoord.x);
	vs_world_position = lin_interpolate(p1, p2, gl_TessCoord.y);

    vs_world_position += vec4(vec3(wave_offset(vs_world_position[0], vs_world_position[2])), 1.0);

	gl_Position = view * vs_world_position;
	vs_light_direction = -gl_Position + view * light_position;
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

    float scale = dot(cross(p1 - p0, p2 - p0), cross(p1 - p0, p2 - p0)); //TODO how to do this?

    int n;
    for (n = 0; n < gl_in.length(); n++) {
		light_direction = vs_light_direction[n];
		gl_Position = projection * gl_in[n].gl_Position;
		world_position = vs_world_position[n];
        position = gl_in[n].gl_Position;
        edge_dist = vec3(float(n == 0), float(n == 1), float(n == 2));
        /*TODO scale properly
        if (scale < 1) {
            edge_dist = vec3(float(n == 0), float(n == 1), float(n == 2)) / scale;
        } else {
            edge_dist = vec3(float(n == 0), float(n == 1), float(n == 2)) * scale;
        }*/
		EmitVertex();
	}
    EndPrimitive();
}
)zzz";

const char* ocean_gs =
menger_gs;
/*R"zzz(#version 330 core
layout (triangles) in;
layout (triangle_strip, max_vertices = 3) out;
uniform mat4 projection;

in vec4 vs_light_direction[];
in vec4 vs_world_position[];

flat out vec4 normal;
flat out vec4 world_normal;
out vec4 light_direction;
out vec4 world_position;
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

    float scale = dot(cross(p1 - p0, p2 - p0), cross(p1 - p0, p2 - p0)); //TODO how to do this?

    int n;
    for (n = 0; n < gl_in.length(); n++) {
		light_direction = vs_light_direction[n];
		gl_Position = projection * gl_in[n].gl_Position;
		world_position = vs_world_position[n];
        edge_dist = vec3(float(n == 0), float(n == 1), float(n == 2));
        //TODO scale properly
		EmitVertex();
	}
    EndPrimitive();
}
)zzz";*/

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
    if (render_wireframe && (edge_dist[0] < 0.02 || edge_dist[1] < 0.02 || edge_dist[2] < 0.02)) {
        fragment_color = vec4(0.0, 1.0, 0.0, 1.0);
    } else {
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
flat in vec4 normal;
in vec4 light_direction;
in vec4 position;
in vec4 world_position;
in vec3 edge_dist;

out vec4 fragment_color;

void main() {
    if (render_wireframe && (edge_dist[0] < 0.02 || edge_dist[1] < 0.02 || edge_dist[2] < 0.02)) {
       fragment_color = vec4(0.0, 1.0, 0.0, 1.0);
    } else {
        // fragment_color = vec4(0.0, 0.0, 1.0, 1.0);

        vec4 ka = vec4(0.0001, 0.0001, 0.0001, 1.0);
        vec4 kd = vec4(0.1, 0.1, 0.5, 1.0);
        vec4 ks = vec4(0.1, 0.1, 0.1, 1.0);
        float alpha = 2000;

        vec4 light_col = vec4(1.0, 1.0, 1.0, 0.0);

        vec4 ambient =  ka * light_col;
        vec4 diffuse =  kd * dot(light_direction, normal);
        vec4 specular =  ks * pow(dot(light_direction, reflect(position, normal)), alpha);

        //distance attenuate?
        fragment_color = ambient + light_col * (diffuse + specular);
    }
})zzz";
