#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <chrono>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/string_cast.hpp>

// OpenGL library includes
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <debuggl.h>

#include "menger.h"
#include "ocean.h"
#include "sphere.h"
#include "ship.h"
#include "camera.h"
#include "shaders.h"

int window_width = 800, window_height = 600;

// VBO and VAO descriptors.
enum { kVertexBuffer, kIndexBuffer, kNumVbos };

// These are our VAOs.
enum { kMengerVao, kFloorVao, kOceanVao, kLightVao, kShipVao, kNumVaos };

GLuint g_array_objects[kNumVaos];  // This will store the VAO descriptors.
GLuint g_buffer_objects[kNumVaos][kNumVbos];  // These will store VBO descriptors.

/*********************************************************/
/*** easier uniform passing ******************************/
#define ULNAME(PREFIX, NAME) PREFIX ## _ ## NAME ## _location
#define GET_UNIFORM_LOC(PREFIX, NAME) GLint ULNAME(PREFIX, NAME) = 0; CHECK_GL_ERROR(ULNAME(PREFIX, NAME) = glGetUniformLocation(PREFIX ## _program_id, #NAME));

#define VAO(NAME) k ## NAME ## Vao
#define BASE_VAO_SETUP(NAME, VERT_DIMEN, FACE_VERTS, VEC_PREFIX) CHECK_GL_ERROR(glGenVertexArrays(kNumVaos, &g_array_objects[VAO(NAME)]));\
CHECK_GL_ERROR(glBindVertexArray(g_array_objects[VAO(NAME)]));\
CHECK_GL_ERROR(glGenBuffers(kNumVbos, &g_buffer_objects[VAO(NAME)][0]));\
CHECK_GL_ERROR(glBindBuffer(GL_ARRAY_BUFFER, g_buffer_objects[VAO(NAME)][kVertexBuffer]));\
CHECK_GL_ERROR(glBufferData(GL_ARRAY_BUFFER, sizeof(float) * VEC_PREFIX ## _vertices.size() * VERT_DIMEN, VEC_PREFIX ## _vertices.data(), GL_STATIC_DRAW));\
CHECK_GL_ERROR(glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0));\
CHECK_GL_ERROR(glEnableVertexAttribArray(0));\
CHECK_GL_ERROR(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_buffer_objects[VAO(NAME)][kIndexBuffer]));\
CHECK_GL_ERROR(glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint32_t) * VEC_PREFIX ## _faces.size() * FACE_VERTS, VEC_PREFIX ## _faces.data(), GL_STATIC_DRAW))


/*********************************************************/
/*** ??? *************************************************/

void CreateTriangle(std::vector<glm::vec4>& vertices, std::vector<glm::uvec3>& indices) {
	vertices.push_back(glm::vec4(-0.5f, -0.5f, -0.5f, 1.0f));
	vertices.push_back(glm::vec4(0.5f, -0.5f, -0.5f, 1.0f));
	vertices.push_back(glm::vec4(0.0f, 0.5f, -0.5f, 1.0f));
	indices.push_back(glm::uvec3(0, 1, 2));
}

void getFloor(std::vector<glm::vec4>& vertices, std::vector<glm::uvec3>& indices, int mode) {
	float L=-10.0f, R=10.0f;
	if(mode == 0) {
		// infinite floor
		vertices.push_back(glm::vec4(0.0f, -2.0f, 0.0f, 1.0f));
		vertices.push_back(glm::vec4(L, -2.0f, L, 0.0f));
		vertices.push_back(glm::vec4(L, -2.0f, R, 0.0f));
		vertices.push_back(glm::vec4(R, -2.0f, R, 0.0f));
		vertices.push_back(glm::vec4(R, -2.0f, L, 0.0f));
		indices.push_back(glm::uvec3(0, 1, 2));
		indices.push_back(glm::uvec3(0, 2, 3));
		indices.push_back(glm::uvec3(0, 3, 4));
		indices.push_back(glm::uvec3(0, 4, 1));
	} else if(mode == 1) {
		// two triangles
		vertices.push_back(glm::vec4(L, -2.0f, L, 1.0f));
		vertices.push_back(glm::vec4(L, -2.0f, R, 1.0f));
		vertices.push_back(glm::vec4(R, -2.0f, R, 1.0f));
		vertices.push_back(glm::vec4(R, -2.0f, L, 1.0f));
		indices.push_back(glm::uvec3(3, 0, 1));
		indices.push_back(glm::uvec3(1, 2, 3));
	}
}

void getFloorQuad(std::vector<glm::vec4>& vertices, std::vector<glm::uvec4>& indices) {
	vertices.clear();
	indices.clear();

	float L=-20.0f, R=20.0f;
	int N = 16;
	int index[N][N], nid=0;
	for(int i=0; i<N+1; i++)
		for(int j=0; j<N+1; j++) {
			vertices.push_back(glm::vec4((R-L)*(i/float(N)) + L, -2.0f,
											   (R-L)*(j/float(N)) + L, 1.0f));
			index[i][j] = nid++;
		}
	for(int i=0; i<N-1; i++)
		for(int j=0; j<N-1; j++)
			indices.push_back(glm::uvec4(index[i+1][j],
										index[i][j],
										index[i+1][j+1],
										index[i][j+1]));
}

void SaveObj(const std::string& file,
	const std::vector<glm::vec4>& vertices,
	const std::vector<glm::uvec3>& indices) {

	std::ofstream fout;
	fout.open(file);
	for(const auto& v: vertices)
		fout << "v " << v[0] << " " << v[1] << " " << v[2] << std::endl;
	for(const auto& f: indices)
		fout << "f " << (1+f[0]) << " " << (1+f[1]) << " " << (1+f[2]) << std::endl;
}

void ErrorCallback(int error, const char* description) {
	std::cerr << "GLFW Error: " << description << "\n";
}

std::shared_ptr<Menger> g_menger;
std::shared_ptr<Ocean> g_ocean;
Camera g_camera;
bool smooth_ctrl = false;
bool enable_ocean = false;
bool tidal_reset = false;
float ELAPSED = 1.0;

float tcs_in_deg = 4.0;
float tcs_out_deg = 4.0;

bool g_render_wireframe = true;
bool g_render_base = true;
int g_init_wave = 0;
bool g_wave_type = false;
bool g_render_lights = false;

bool g_launch_ships = false;

auto g_lt = std::chrono::system_clock::now();

enum class MovementDirection {
    FORWARD = 1, BACKWARD = -1, NONE = 0, FAST_FORW = 3, FAST_BACK = -3
};
MovementDirection g_should_move = MovementDirection::NONE;
enum class PanDirection {
    P = 1, N = -1, NONE = 0, FAST_P = 3, FAST_N = -3
};
PanDirection g_should_pan_x = PanDirection::NONE;
PanDirection g_should_pan_y = PanDirection::NONE;
enum class RollDirection {
    COUNTER = -1, CLOCK = 1, NONE = 0, FAST_COUNTER = -3, FAST_CLOCK = -3
};
RollDirection g_should_roll = RollDirection::NONE;

/*********************************************************/
/*** Keyboard ********************************************/

void KeyCallback(GLFWwindow* window,
	int key,
	int scancode,
	int action,
	int mods
) {
	// Note:
	// This is only a list of functions to implement.
	// you may want to re-organize this piece of code.
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
		glfwSetWindowShouldClose(window, GL_TRUE);
	} else if (key == GLFW_KEY_S && mods == GLFW_MOD_CONTROL && action == GLFW_RELEASE) {
		std::vector<glm::vec4> obj_vertices;
		std::vector<glm::uvec3> obj_faces;
        g_menger->generate_geometry(obj_vertices, obj_faces);
		SaveObj(std::string("geometry.obj"), obj_vertices, obj_faces);
		std::cout << "saved model to geometry.obj" << std::endl;
	} else if (key == GLFW_KEY_W) { // move forwards
        if (action == GLFW_RELEASE && g_should_move == MovementDirection::FORWARD) {
            g_should_move = MovementDirection::NONE;
        } else if (action == GLFW_PRESS) {
    		g_should_move = MovementDirection::FORWARD;
        }
        if(!smooth_ctrl) g_camera.move(ELAPSED, (int) g_should_move);
	} else if (key == GLFW_KEY_S) { // move backwards
        if (action == GLFW_RELEASE && g_should_move < MovementDirection::NONE) {
            g_should_move = MovementDirection::NONE;
        } else if (action == GLFW_PRESS) {
            g_should_move = MovementDirection::BACKWARD;
        }
        if(!smooth_ctrl) g_camera.move(ELAPSED, (int) g_should_move);
	} else if (key == GLFW_KEY_A) { // pan left
        if (action == GLFW_RELEASE && g_should_pan_x == PanDirection::N) {
            g_should_pan_x = PanDirection::NONE;
        } else if (action == GLFW_PRESS) {
            g_should_pan_x = PanDirection::N;
        }
        if(!smooth_ctrl) g_camera.pan_x(ELAPSED, (int) g_should_pan_x);
	} else if (key == GLFW_KEY_D) { // pan right
        if (action == GLFW_RELEASE && g_should_pan_x == PanDirection::P) {
            g_should_pan_x = PanDirection::NONE;
        } else if (action == GLFW_PRESS) {
            g_should_pan_x = PanDirection::P;
        }
        if(!smooth_ctrl) g_camera.pan_x(ELAPSED, (int) g_should_pan_x);
	} else if (key == GLFW_KEY_LEFT) { // roll counter
        if (action == GLFW_RELEASE && g_should_roll == RollDirection::COUNTER) {
            g_should_roll = RollDirection::NONE;
        } else if (action == GLFW_PRESS) {
            g_should_roll = RollDirection::COUNTER;
        }
        if(!smooth_ctrl) g_camera.roll(ELAPSED, (int) g_should_roll);
	} else if (key == GLFW_KEY_RIGHT) { // roll clockwise
        if (action == GLFW_RELEASE && g_should_roll == RollDirection::CLOCK) {
            g_should_roll = RollDirection::NONE;
        } else if (action == GLFW_PRESS) {
            g_should_roll = RollDirection::CLOCK;
        }
        if(!smooth_ctrl) g_camera.roll(ELAPSED, (int) g_should_roll);
	} else if (key == GLFW_KEY_DOWN) { // pan up
        if (action == GLFW_RELEASE && g_should_pan_y == PanDirection::P) {
            g_should_pan_y = PanDirection::NONE;
        } else if (action == GLFW_PRESS) {
            g_should_pan_y = PanDirection::P;
        }
        if(!smooth_ctrl) g_camera.pan_y(ELAPSED, (int) g_should_pan_y);
	} else if (key == GLFW_KEY_UP) { // pan down
        if (action == GLFW_RELEASE && g_should_pan_y == PanDirection::N) {
            g_should_pan_y = PanDirection::NONE;
        } else if (action == GLFW_PRESS) {
            g_should_pan_y = PanDirection::N;
        }
        if(!smooth_ctrl) g_camera.pan_y(ELAPSED, (int) g_should_pan_y);
	} else if (key == GLFW_KEY_M && action == GLFW_PRESS && mods == GLFW_MOD_CONTROL) {
        smooth_ctrl = !smooth_ctrl;
    } else if (key == GLFW_KEY_C && action == GLFW_PRESS) {
		g_camera.change_mode();
    } else if (key == GLFW_KEY_F && action == GLFW_PRESS) {
        if (mods == GLFW_MOD_CONTROL) {
            g_render_base = !g_render_base;
        } else {
            g_render_wireframe = !g_render_wireframe;
        }
	} else if (key == GLFW_KEY_MINUS && action != GLFW_RELEASE) { // outer -
		if (tcs_out_deg > 1.0) tcs_out_deg -= 1.0;
	} else if (key == GLFW_KEY_EQUAL && action != GLFW_RELEASE) { // outer +
		tcs_out_deg += 1.0;
	} else if (key == GLFW_KEY_COMMA && action != GLFW_RELEASE) { // inner -
		if (tcs_in_deg > 1.0) tcs_in_deg -= 1.0;
	} else if (key == GLFW_KEY_PERIOD && action != GLFW_RELEASE) { // inner +
		tcs_in_deg += 1.0;
	} else if (key == GLFW_KEY_O && mods == GLFW_MOD_CONTROL && action == GLFW_RELEASE) {
		enable_ocean = !enable_ocean;
	} else if (key == GLFW_KEY_T && mods == GLFW_MOD_CONTROL && action == GLFW_RELEASE) {
        tidal_reset = true;
    } else if (key == GLFW_KEY_L && mods == GLFW_MOD_CONTROL && action == GLFW_RELEASE) {
        g_render_lights = !g_render_lights;
    } else if (key == GLFW_KEY_L && action == GLFW_RELEASE) {
        g_launch_ships = !g_launch_ships;
    } else if (key == GLFW_KEY_R && action == GLFW_PRESS) {
        g_camera.reset();
    }
	if (!g_menger) return; // 0-4 only available in Menger mode.
	if (key == GLFW_KEY_0 && action != GLFW_RELEASE) {
		g_menger->set_nesting_level(0);
	} else if (key == GLFW_KEY_1 && action != GLFW_RELEASE) {
        g_menger->set_nesting_level(1);
	} else if (key == GLFW_KEY_2 && action != GLFW_RELEASE) {
        g_menger->set_nesting_level(2);
	} else if (key == GLFW_KEY_3 && action != GLFW_RELEASE) {
        g_menger->set_nesting_level(3);
	} else if (key == GLFW_KEY_4 && action != GLFW_RELEASE) {
        g_menger->set_nesting_level(4);
	}
}

/*********************************************************/
/*** Mouse ***********************************************/

//mouse
std::vector<int> g_buttons = std::vector<int>();
int g_curr_button;
bool g_mouse_pressed = false;
bool g_mouse_init = false;
int g_mouse_last_x;
int g_mouse_last_y;

void MousePosCallback(GLFWwindow* window, double mouse_x, double mouse_y) {
    if (!g_mouse_init && g_mouse_pressed) {
        g_mouse_last_x = mouse_x;
        g_mouse_last_y = mouse_y;
        g_mouse_init = true;
    }
	if (!g_mouse_pressed) return;
    double dx = mouse_x - g_mouse_last_x;
    double dy = mouse_y - g_mouse_last_y;
	if (g_curr_button == GLFW_MOUSE_BUTTON_LEFT) {
        g_camera.mouse_rot(dx, dy);
	} else if (g_curr_button == GLFW_MOUSE_BUTTON_RIGHT) {
        g_camera.mouse_zoom(dx, dy);
	} else if (g_curr_button == GLFW_MOUSE_BUTTON_MIDDLE) {
        g_camera.mouse_strafe(dx, dy);
	}
    g_mouse_last_x = mouse_x;
    g_mouse_last_y = mouse_y;
}

void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    if (action == GLFW_PRESS) {
        g_curr_button = button;
        g_mouse_pressed = true;
        g_buttons.push_back(button);
    } else {
        for (auto it = g_buttons.begin(); it != g_buttons.end(); ++it) {
            if (*it == button) {
                g_buttons.erase(it);
                break;
            }
        }
        if (g_buttons.size() == 0) {
            g_mouse_init = false;
            g_mouse_pressed = false;
        } else {
            g_curr_button = g_buttons.back();
        }
    }
}

int main(int argc, char* argv[]) {

	if(argc > 1 && argv[1][0] == '-' && argv[1][1] == 's') // TODO: put this in right place?
		smooth_ctrl = true;

	std::string window_title = "Menger";
	if (!glfwInit()) exit(EXIT_FAILURE);
	glfwSetErrorCallback(ErrorCallback);

	/*********************************************************/
	/*** Models **********************************************/

	// menger
	g_menger = std::make_shared<Menger>();
	std::vector<glm::vec4> obj_vertices;
	std::vector<glm::uvec3> obj_faces;
	g_menger->set_nesting_level(1);
	g_menger->generate_geometry(obj_vertices, obj_faces);
	g_menger->set_clean();

	// floor
	std::vector<glm::vec4> floor_vertices;
	std::vector<glm::uvec3> floor_faces;
	getFloor(floor_vertices, floor_faces, 1);

	// ocean
    g_ocean = std::make_shared<Ocean>();
	std::vector<glm::vec4> ocean_vertices;
	std::vector<glm::uvec4> ocean_faces;
    g_ocean->generate_geometry(ocean_vertices, ocean_faces);

    // light
    std::vector<glm::vec4> light_vertices;
	std::vector<glm::uvec3> light_faces;
    sphere::create_sphere(1.0, 10, 10, light_vertices, light_faces);

    // ships
    std::vector<glm::vec4> ship_vertices;
	std::vector<glm::uvec3> ship_faces;
    ship::generate_geometry(ship_vertices, ship_faces);

	/*********************************************************/
	/*** OpenGL: Context  ************************************/

	// Ask an OpenGL 4.1 core profile context
	// It is required on OSX and non-NVIDIA Linux
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	GLFWwindow* window = glfwCreateWindow(window_width, window_height, &window_title[0], nullptr, nullptr);
	CHECK_SUCCESS(window != nullptr);
	glfwMakeContextCurrent(window);
	glewExperimental = GL_TRUE;

	CHECK_SUCCESS(glewInit() == GLEW_OK);
	glGetError();  // clear GLEW's error for it
	glfwSetKeyCallback(window, KeyCallback);
	glfwSetCursorPosCallback(window, MousePosCallback);
	glfwSetMouseButtonCallback(window, MouseButtonCallback);
	glfwSwapInterval(1);
	const GLubyte* renderer = glGetString(GL_RENDERER);  // get renderer string
	const GLubyte* version = glGetString(GL_VERSION);    // version as a string
	std::cout << "Renderer: " << renderer << "\n";
	std::cout << "OpenGL version supported:" << version << "\n";

    // CreateTriangle(obj_vertices, obj_faces);
	glm::vec4 min_bounds = glm::vec4(std::numeric_limits<float>::max());
	glm::vec4 max_bounds = glm::vec4(-std::numeric_limits<float>::max());
	for (const auto& vert : obj_vertices) {
		min_bounds = glm::min(vert, min_bounds);
		max_bounds = glm::max(vert, max_bounds);
	}
	std::cout << "min_bounds = " << glm::to_string(min_bounds) << "\n";
	std::cout << "max_bounds = " << glm::to_string(max_bounds) << "\n";

	/*********************************************************/
	/*** OpenGL: VAO + VBO  **********************************/

	/*** Geometry Program ***/
    BASE_VAO_SETUP(Menger, 4, 3, obj);
	/*** Floor Program ***/
    BASE_VAO_SETUP(Floor, 4, 3, floor);
	/*** Ocean Program ***/
    BASE_VAO_SETUP(Ocean, 4, 4, ocean);
    /*** Light Program ***/
    BASE_VAO_SETUP(Light, 4, 3, light);
    /*** Ship Program(s) ***/
    BASE_VAO_SETUP(Ship, 4, 3, ship);

	/*********************************************************/
	/*** OpenGL: Shaders & Programs **************************/

	/*** Geometry Program ***/

    std::cout << "Compiling menger program." << std::endl;

	// Let's create our program.
	GLuint program_id = shaders::menger_sss.compile().create_program();

	// Bind attributes.
	CHECK_GL_ERROR(glBindAttribLocation(program_id, 0, "w_pos"));
	CHECK_GL_ERROR(glBindFragDataLocation(program_id, 0, "frag_col"));
	glLinkProgram(program_id);
	CHECK_GL_PROGRAM_ERROR(program_id);

	// Get the uniform locations.
	GLint projection_matrix_location = 0;
	CHECK_GL_ERROR(projection_matrix_location =
		glGetUniformLocation(program_id, "projection"));
	GLint view_matrix_location = 0;
	CHECK_GL_ERROR(view_matrix_location =
		glGetUniformLocation(program_id, "view"));
	GLint light_position_location = 0;
	CHECK_GL_ERROR(light_position_location =
		glGetUniformLocation(program_id, "w_lpos"));
	GLint render_wireframe_location = 0;
	CHECK_GL_ERROR(render_wireframe_location =
		glGetUniformLocation(program_id, "render_wireframe"));


	/*** Floor Program ***/

    std::cout << "Compiling floor program." << std::endl;
	// create program
	GLuint floor_program_id = shaders::floor_sss.compile().create_program();

	// bind attributes
	CHECK_GL_ERROR(glBindAttribLocation(floor_program_id, 0, "w_pos"));
	CHECK_GL_ERROR(glBindFragDataLocation(floor_program_id, 0, "frag_col"));
	glLinkProgram(floor_program_id);
	CHECK_GL_PROGRAM_ERROR(floor_program_id);

	// unifrom locations
	GLint floor_projection_matrix_location = 0;
	CHECK_GL_ERROR(floor_projection_matrix_location =
			glGetUniformLocation(floor_program_id, "projection"));
	GLint floor_view_matrix_location = 0;
	CHECK_GL_ERROR(floor_view_matrix_location =
			glGetUniformLocation(floor_program_id, "view"));
	GLint floor_light_position_location = 0;
	CHECK_GL_ERROR(floor_light_position_location =
			glGetUniformLocation(floor_program_id, "w_lpos"));
	GLint tcs_in_deg_location = 0;
	CHECK_GL_ERROR(tcs_in_deg_location =
			glGetUniformLocation(floor_program_id, "tcs_in_deg"));
	GLint tcs_out_deg_location = 0;
	CHECK_GL_ERROR(tcs_out_deg_location =
			glGetUniformLocation(floor_program_id, "tcs_out_deg"));
	GLint floor_render_wireframe_location = 0;
	CHECK_GL_ERROR(floor_render_wireframe_location =
	        glGetUniformLocation(floor_program_id, "render_wireframe"));


	/*** Ocean Program ***/

    std::cout << "Compiling ocean program." << std::endl;
	// create program
	GLuint ocean_program_id = shaders::ocean_sss.compile().create_program();

	// bind attributes
	CHECK_GL_ERROR(glBindAttribLocation(ocean_program_id, 0, "w_pos"));
	CHECK_GL_ERROR(glBindFragDataLocation(ocean_program_id, 0, "frag_col"));
	glLinkProgram(ocean_program_id);
	CHECK_GL_PROGRAM_ERROR(ocean_program_id);

	// unifrom locations
    GET_UNIFORM_LOC(ocean, projection);
    GET_UNIFORM_LOC(ocean, view);
    GET_UNIFORM_LOC(ocean, w_lpos);
    GET_UNIFORM_LOC(ocean, tcs_in_deg);
    GET_UNIFORM_LOC(ocean, tcs_out_deg);
    GET_UNIFORM_LOC(ocean, wave_time);
    GET_UNIFORM_LOC(ocean, wave_type);
    GET_UNIFORM_LOC(ocean, tidal_time);
    GET_UNIFORM_LOC(ocean, render_wireframe);
    GET_UNIFORM_LOC(ocean, cterm);
    GET_UNIFORM_LOC(ocean, lterm);
    GET_UNIFORM_LOC(ocean, qterm);
    GET_UNIFORM_LOC(ocean, ka);
    GET_UNIFORM_LOC(ocean, kd);
    GET_UNIFORM_LOC(ocean, ks);
    GET_UNIFORM_LOC(ocean, alpha);

    /*** light program ***/
    std::cout << "Compiling light program." << std::endl;
	// create program
	GLuint light_program_id = shaders::light_sss.compile().create_program();
	// bind attributes
	CHECK_GL_ERROR(glBindAttribLocation(light_program_id, 0, "w_pos"));
	CHECK_GL_ERROR(glBindFragDataLocation(light_program_id, 0, "frag_col"));
	glLinkProgram(light_program_id);
	CHECK_GL_PROGRAM_ERROR(light_program_id);
	// unifrom locations
    GET_UNIFORM_LOC(light, projection);
    GET_UNIFORM_LOC(light, view);
    GET_UNIFORM_LOC(light, model);
    GET_UNIFORM_LOC(light, w_lpos);
    GET_UNIFORM_LOC(light, render_wireframe);

    /*** light program ***/
    std::cout << "Compiling ship program." << std::endl;
	// create program
	GLuint ship_program_id = shaders::ship_sss.compile().create_program();
	// bind attributes
	CHECK_GL_ERROR(glBindAttribLocation(ship_program_id, 0, "w_pos"));
	CHECK_GL_ERROR(glBindFragDataLocation(ship_program_id, 0, "frag_col"));
	glLinkProgram(ship_program_id);
	CHECK_GL_PROGRAM_ERROR(ship_program_id);
	// unifrom locations
    GET_UNIFORM_LOC(ship, projection);
    GET_UNIFORM_LOC(ship, view);
    GET_UNIFORM_LOC(ship, model);
    GET_UNIFORM_LOC(ship, w_lpos);
    GET_UNIFORM_LOC(ship, render_wireframe);


	/*********************************************************/
	/*** OpenGL: Uniforms ************************************/
    int level = 0;
	glm::vec4 light_position = glm::vec4(-10.0f, 10.0f, 0.0f, 1.0f);
    auto light_model_matrix = glm::translate(glm::mat4(1.0f), glm::vec3(light_position));
	float aspect = 0.0f;
	float theta = 0.0f;
    auto start = std::chrono::system_clock::now();
    g_lt = std::chrono::system_clock::now();

    // Material data
    auto cterm = 0.25;
    auto lterm = 0.003372407;
    auto qterm = 0.000045492;
    auto ocean_ka = glm::vec3(0.05, 0.05, 0.15);
    auto ocean_kd = glm::vec3(0.1, 0.1, 0.3);
    auto ocean_ks = glm::vec3(1.0, 1.0, 1.0);
    auto ocean_alpha = 20.0f;

    //ocean data group
    fluid::ocean_surf_params ocean_data {
        std::vector<fluid::wave_params> {{
            fluid::wave_params(0.5, 4, -0.005, 5, glm::vec2(1, 1))
        }},
        fluid::gaussian_params {
            glm::vec2(0.001, 0),
            glm::vec2(-10, 0),
            40.0,
            1.0,
            std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::minutes(1)).count()
        },
        std::vector<fluid::wave_packet> {}
    };

    // Ship
    std::vector<ship::instance> ship_instances {{
        ship::instance {
            false, // side
            glm::vec4(0.0f, -2.0f, 0.0f, 1.0f),
            glm::vec4(0.0f, 0.0f, 0.0f, 0.0f),
            glm::vec3(0.0f, 1.0f, 0.0f),
            glm::vec3(0.0f, 0.0f, -1.0f)
        }
    }};

	while (!glfwWindowShouldClose(window)) {

        /*********************************************************/
		/*** Delta time smoothing ********************************/

		auto ct = std::chrono::system_clock::now();
        double elapsed = (ct - g_lt).count();
        double since_start = std::chrono::duration_cast<std::chrono::milliseconds>(ct - start).count();
        double tidal_since_start = since_start - ocean_data.gp.start;
        g_lt = ct;

		/*********************************************************/
		/*** OpenGL: Clear ***************************************/

		// Setup some basic window stuff.
		glfwGetFramebufferSize(window, &window_width, &window_height);
		glViewport(0, 0, window_width, window_height);
		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		glEnable(GL_DEPTH_TEST);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glDepthFunc(GL_LESS);

		/*********************************************************/
		/*** OpenGL: Regenerate **********************************/

		// Switch to the Geometry VAO.
		if (g_menger && g_menger->is_dirty()) {
            g_menger->generate_geometry(obj_vertices, obj_faces);
			g_menger->set_clean();

            obj_vertices.push_back(light_position + glm::vec4(0.01f, 0.01f, 0.01f, 1.0f));
            obj_vertices.push_back(light_position + glm::vec4(-0.01f, -0.01f, 0.01f, 1.0f));
            obj_vertices.push_back(light_position + glm::vec4(0.01f, -0.01f, 0.01f, 1.0f));
            obj_faces.push_back(glm::uvec3(obj_vertices.size() - 3, obj_vertices.size() - 2, obj_vertices.size() - 1));

            CHECK_GL_ERROR(glBindVertexArray(g_array_objects[kMengerVao]));

            CHECK_GL_ERROR(glBindBuffer(GL_ARRAY_BUFFER, g_buffer_objects[kMengerVao][kVertexBuffer]));
            CHECK_GL_ERROR(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_buffer_objects[kMengerVao][kIndexBuffer]));

            CHECK_GL_ERROR(glBufferData(GL_ARRAY_BUFFER, sizeof(float) * obj_vertices.size() * 4, obj_vertices.data(), GL_STATIC_DRAW));
            CHECK_GL_ERROR(glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint32_t) * obj_faces.size() * 3, obj_faces.data(), GL_STATIC_DRAW));
		}
        if (enable_ocean && g_ocean->dirty()) {
            CHECK_GL_ERROR(glBindVertexArray(g_array_objects[kOceanVao]));

            g_ocean->generate_geometry(ocean_vertices, ocean_faces);

            CHECK_GL_ERROR(glBindBuffer(GL_ARRAY_BUFFER, g_buffer_objects[kOceanVao][kVertexBuffer]));
            CHECK_GL_ERROR(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_buffer_objects[kOceanVao][kIndexBuffer]));

            CHECK_GL_ERROR(glBufferData(GL_ARRAY_BUFFER, sizeof(float) * obj_vertices.size() * 4, ocean_vertices.data(), GL_STATIC_DRAW));
            CHECK_GL_ERROR(glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint32_t) * obj_faces.size() * 4, ocean_faces.data(), GL_STATIC_DRAW));
        }

		/*********************************************************/
		/*** OpenGL: Light + Camera ******************************/

		// Compute the projection matrix.
		aspect = static_cast<float>(window_width) / window_height;
		glm::mat4 projection_matrix =
			glm::perspectiveFov(g_camera.get_fov(45.0f), (float) window_width, (float) window_height, 0.0001f, 1000.0f);
		// Compute the view matrix
		glm::mat4 view_matrix = g_camera.get_view_matrix();

		/*********************************************************/
		/*** OpenGL: Render  *************************************/

        /** Universal settings ***/
        glPolygonMode(GL_FRONT_AND_BACK, g_render_base ? GL_FILL : GL_LINE);

		/*** Menger Program ***/
		// Use our program.
		CHECK_GL_ERROR(glUseProgram(program_id));
		// Draw our triangles.
        CHECK_GL_ERROR(glBindVertexArray(g_array_objects[kMengerVao]));

		// Pass uniforms in.
		CHECK_GL_ERROR(glUniformMatrix4fv(projection_matrix_location, 1, GL_FALSE, &projection_matrix[0][0]));
		CHECK_GL_ERROR(glUniformMatrix4fv(view_matrix_location, 1, GL_FALSE, &view_matrix[0][0]));
		CHECK_GL_ERROR(glUniform4fv(light_position_location, 1, &light_position[0]));
        CHECK_GL_ERROR(glUniform1i(render_wireframe_location, g_render_wireframe));

        // draw
		CHECK_GL_ERROR(glDrawElements(GL_TRIANGLES, obj_faces.size() * 3, GL_UNSIGNED_INT, 0));

		if(!enable_ocean) { /*** Floor Program ***/
			// set program + vao
			CHECK_GL_ERROR(glUseProgram(floor_program_id));
			CHECK_GL_ERROR(glBindVertexArray(g_array_objects[kFloorVao]));

			// pass uniforms
			CHECK_GL_ERROR(glUniformMatrix4fv(floor_projection_matrix_location, 1, GL_FALSE,
				&projection_matrix[0][0]));
			CHECK_GL_ERROR(glUniformMatrix4fv(floor_view_matrix_location, 1, GL_FALSE,
				&view_matrix[0][0]));
			CHECK_GL_ERROR(glUniform4fv(floor_light_position_location, 1, &light_position[0]));
			CHECK_GL_ERROR(glUniform1f(tcs_in_deg_location, tcs_in_deg));
			CHECK_GL_ERROR(glUniform1f(tcs_out_deg_location, tcs_out_deg));
            CHECK_GL_ERROR(glUniform1i(floor_render_wireframe_location, g_render_wireframe));

			// Render floor
			CHECK_GL_ERROR(glPatchParameteri(GL_PATCH_VERTICES, 3));
			CHECK_GL_ERROR(glDrawElements(GL_PATCHES, floor_faces.size() * 3, GL_UNSIGNED_INT, 0));
			// CHECK_GL_ERROR(glDrawElements(GL_TRIANGLES, floor_faces.size() * 3, GL_UNSIGNED_INT, 0));
			// CHECK_GL_ERROR(glDrawArrays(GL_PATCHES, 0, floor_faces.size() * 3)); // ???
		} else { /*** Ocean Program ***/
			// set program + vao
			CHECK_GL_ERROR(glUseProgram(ocean_program_id));
			CHECK_GL_ERROR(glBindVertexArray(g_array_objects[kOceanVao]));

			// pass uniforms
			CHECK_GL_ERROR(glUniformMatrix4fv(ULNAME(ocean, projection), 1, GL_FALSE, &projection_matrix[0][0]));
			CHECK_GL_ERROR(glUniformMatrix4fv(ULNAME(ocean, view), 1, GL_FALSE, &view_matrix[0][0]));
			CHECK_GL_ERROR(glUniform4fv(ULNAME(ocean, w_lpos), 1, &light_position[0]));
			CHECK_GL_ERROR(glUniform1f(ULNAME(ocean, tcs_in_deg), tcs_in_deg));
			CHECK_GL_ERROR(glUniform1f(ULNAME(ocean, tcs_out_deg), tcs_out_deg));
            CHECK_GL_ERROR(glUniform1f(ULNAME(ocean, wave_time), since_start));
            CHECK_GL_ERROR(glUniform1i(ULNAME(ocean, wave_type), g_wave_type));
            CHECK_GL_ERROR(glUniform1f(ULNAME(ocean, tidal_time), tidal_since_start));
            CHECK_GL_ERROR(glUniform1i(ULNAME(ocean, render_wireframe), g_render_wireframe));
            CHECK_GL_ERROR(glUniform1f(ULNAME(ocean, cterm), cterm));
            CHECK_GL_ERROR(glUniform1f(ULNAME(ocean, lterm), lterm));
            CHECK_GL_ERROR(glUniform1f(ULNAME(ocean, qterm), qterm));
            CHECK_GL_ERROR(glUniform3fv(ULNAME(ocean, ka), 1, &ocean_ka[0]));
            CHECK_GL_ERROR(glUniform3fv(ULNAME(ocean, kd), 1, &ocean_kd[0]));
            CHECK_GL_ERROR(glUniform3fv(ULNAME(ocean, ks), 1, &ocean_ks[0]));
            CHECK_GL_ERROR(glUniform1f(ULNAME(ocean, alpha), ocean_alpha));

			// Render floor
			CHECK_GL_ERROR(glPatchParameteri(GL_PATCH_VERTICES, 4));
			CHECK_GL_ERROR(glDrawElements(GL_PATCHES, ocean_faces.size() * 4, GL_UNSIGNED_INT, 0));
		}

        if (g_render_lights) {
			CHECK_GL_ERROR(glUseProgram(light_program_id));
			CHECK_GL_ERROR(glBindVertexArray(g_array_objects[kLightVao]));
			CHECK_GL_ERROR(glUniformMatrix4fv(ULNAME(light, projection), 1, GL_FALSE, &projection_matrix[0][0]));
			CHECK_GL_ERROR(glUniformMatrix4fv(ULNAME(light, view), 1, GL_FALSE, &view_matrix[0][0]));
            CHECK_GL_ERROR(glUniformMatrix4fv(ULNAME(light, model), 1, GL_FALSE, &light_model_matrix[0][0]));
            CHECK_GL_ERROR(glUniform4fv(ULNAME(light, w_lpos), 1, &light_position[0]));
            CHECK_GL_ERROR(glUniform1i(ULNAME(light, render_wireframe), g_render_wireframe));
    		CHECK_GL_ERROR(glDrawElements(GL_TRIANGLES, obj_faces.size() * 3, GL_UNSIGNED_INT, 0));
        }

        if (g_launch_ships && enable_ocean) {
            // set program + vao
            CHECK_GL_ERROR(glUseProgram(ship_program_id));
            CHECK_GL_ERROR(glBindVertexArray(g_array_objects[kShipVao]));
            for (auto& instance : ship_instances) {
                auto ship_model_matrix = ship::model_matrix(since_start, instance, ocean_data);
                CHECK_GL_ERROR(glUniformMatrix4fv(ULNAME(ship, projection), 1, GL_FALSE, &projection_matrix[0][0]));
                CHECK_GL_ERROR(glUniformMatrix4fv(ULNAME(ship, view), 1, GL_FALSE, &view_matrix[0][0]));
                CHECK_GL_ERROR(glUniformMatrix4fv(ULNAME(ship, model), 1, GL_FALSE, &ship_model_matrix[0][0]));
                CHECK_GL_ERROR(glUniform4fv(ULNAME(ship, w_lpos), 1, &light_position[0]));
                CHECK_GL_ERROR(glUniform1i(ULNAME(ship, render_wireframe), g_render_wireframe));
                CHECK_GL_ERROR(glDrawElements(GL_TRIANGLES, obj_faces.size() * 3, GL_UNSIGNED_INT, 0));
            }
        }

		/*********************************************************/
		/*** Next Iteration **************************************/

		glfwPollEvents(); // get interaction
		glfwSwapBuffers(window); // swap buffer

		if (smooth_ctrl) { // time delta smoothing
	        auto tdelta = elapsed / 20000000;
	        if ((int) g_should_move) {
	            g_camera.move(tdelta, (int) g_should_move);
	        }
	        if ((int) g_should_roll) {
	            g_camera.roll(tdelta, (int) g_should_roll);
	        }
	        if ((int) g_should_pan_x) {
	            g_camera.pan_x(tdelta, (int) g_should_pan_x);
	        }
	        if ((int) g_should_pan_y) {
	            g_camera.pan_y(tdelta, (int) g_should_pan_y);
	        }
		}

		if(tidal_reset) { // tidal wave
			tidal_reset = false;
            ocean_data.gp.start = since_start;
		}

        for (auto& instance : ship_instances) { // ships
            instance.simulate(elapsed, ship_instances);
        }
	}

	/*********************************************************/
	/*** OpenGL: Clean up ************************************/

	glfwDestroyWindow(window);
	glfwTerminate();
	exit(EXIT_SUCCESS);
}
