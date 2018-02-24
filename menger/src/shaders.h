// C++ 11 String Literal
// See http://en.cppreference.com/w/cpp/language/string_literal

/*********************************************************/
/*** vertex **********************************************/

/*** default ***/

const char* vertex_shader =
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



/*********************************************************/
/*** tesselation *****************************************/

/*** triangle ***/

float tcs_in_deg = 1.0;
float tcs_out_deg = 1.0;
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
}
)zzz";


/*** quadrangle ***/

const char* quad_t_ctrl_shader =
R"zzz(#version 330 core
layout (quads, max_vertices = 4) out;
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
out vec4 vs_light_direction;
out vec4 vs_world_position;
vec4 lin_interpolate(vec4 A, vec4 B, float t) {
	return A*t + B*(1-t);
}
void main(void){ // TODO: check order of vertices
	vec4 p1 = lin_interpolate(gl_in[0].gl_Position, gl_in[1].gl_Position, gl_TessCoord.x);
	vec4 p2 = lin_interpolate(gl_in[2].gl_Position, gl_in[3].gl_Position, gl_TessCoord.x);
	vs_world_position = lin_interpolate(p1, p2, gl_TessCoord.y);
	gl_Position = view * vs_world_position;
	vs_light_direction = -gl_Position + view * light_position;
}
)zzz";



/*********************************************************/
/*** geometry ********************************************/

const char* geometry_shader =
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

// out vec4 color_v;
void main()
{
	int n = 0;
    vec3 wp0 = vec3(vs_world_position[0]);
    vec3 wp1 = vec3(vs_world_position[1]);
    vec3 wp2 = vec3(vs_world_position[2]);
    world_normal = vec4(normalize(cross(wp1 - wp0, wp2 - wp0)), 1.0);
    vec3 p0 = gl_in[0].gl_Position.xyz;
    vec3 p1 = gl_in[1].gl_Position.xyz;
    vec3 p2 = gl_in[2].gl_Position.xyz;
    normal = vec4(normalize(cross(p1 - p0, p2 - p0)), 1.0);
	for (n = 0; n < gl_in.length(); n++) {
		light_direction = vs_light_direction[n];
		gl_Position = projection * gl_in[n].gl_Position;
		world_position = vs_world_position[n];
		// if(n == 0)
		// 	color_v = vec4(1.0, 0.0, 0.0, 1.0);
		// if(n == 1)
		// 	color_v = vec4(0.0, 1.0, 0.0, 1.0);
		// if(n == 2)
		// 	color_v = vec4(0.0, 0.0, 1.0, 1.0);
		EmitVertex();
	}
    EndPrimitive();
}
)zzz";



/*********************************************************/
/*** fragment ********************************************/

const char* fragment_shader =
R"zzz(#version 330 core
flat in vec4 normal;
flat in vec4 world_normal;
in vec4 light_direction;
out vec4 fragment_color;
void main()
{
	vec4 color = clamp(world_normal * world_normal, 0.0, 1.0);
	float dot_nl = dot(normalize(light_direction), normalize(normal));
	dot_nl = clamp(dot_nl, 0.0, 1.0);
	fragment_color = clamp(dot_nl * color, 0.0, 1.0);
}
)zzz";

const char* floor_fragment_shader =
R"zzz(#version 330 core
flat in vec4 normal;
in vec4 light_direction;
in vec4 world_position;
out vec4 fragment_color;

// in vec4 color_v;
void main()
{
	vec4 color = vec4(1.0, 1.0, 1.0, 1.0);
	if (mod(floor(world_position[0])+floor(world_position[2]), 2) == 0) {
		color *= 0;
	}
	// color = color_v;
	float dot_nl = dot(normalize(light_direction), normalize(normal));
	dot_nl = clamp(dot_nl, 0.0, 1.0);
	fragment_color = clamp(dot_nl * color, 0.0, 1.0);
	// fragment_color = vec4(1.0, 0.0, 0.0, 1.0);
}
)zzz";