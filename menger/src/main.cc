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
#include "camera.h"
#include "shaders.h"

int window_width = 800, window_height = 600;

// VBO and VAO descriptors.
enum { kVertexBuffer, kIndexBuffer, kNumVbos };

// These are our VAOs.
enum { kGeometryVao, kFloorVao, kOceanVao, kNumVaos };

GLuint g_array_objects[kNumVaos];  // This will store the VAO descriptors.
GLuint g_buffer_objects[kNumVaos][kNumVbos];  // These will store VBO descriptors.

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

void
ErrorCallback(int error, const char* description) {
	std::cerr << "GLFW Error: " << description << "\n";
}

std::shared_ptr<Menger> g_menger;
Camera g_camera;
bool smooth_ctrl = false;
bool enable_ocean = false;
float ELAPSED = 1.0;

auto g_lt = std::chrono::system_clock::now();

enum class MovementDirection {
    FORWARD = 1, BACKWARD = -1, NONE = 0
};
MovementDirection g_should_move = MovementDirection::NONE;
enum class PanDirection {
    P = 1, N = -1, NONE = 0
};
PanDirection g_should_pan_x = PanDirection::NONE;
PanDirection g_should_pan_y = PanDirection::NONE;
enum class RollDirection {
    COUNTER = -1, CLOCK = 1, NONE = 0
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
    		if(!smooth_ctrl) g_camera.move(ELAPSED, (int) g_should_move);
        }
	} else if (key == GLFW_KEY_S && mods != GLFW_MOD_CONTROL) { // move backwards
        if (action == GLFW_RELEASE && g_should_move == MovementDirection::BACKWARD) {
            g_should_move = MovementDirection::NONE;
        } else if (action == GLFW_PRESS) {
            g_should_move = MovementDirection::BACKWARD;
    		if(!smooth_ctrl) g_camera.move(ELAPSED, (int) g_should_move);
        }
	} else if (key == GLFW_KEY_A) { // pan left
        if (action == GLFW_RELEASE && g_should_pan_x == PanDirection::N) {
            g_should_pan_x = PanDirection::NONE;
        } else if (action == GLFW_PRESS) {
            g_should_pan_x = PanDirection::N;
	        if(!smooth_ctrl) g_camera.pan_x(ELAPSED, (int) g_should_pan_x);
        }
	} else if (key == GLFW_KEY_D) { // pan right
        if (action == GLFW_RELEASE && g_should_pan_x == PanDirection::P) {
            g_should_pan_x = PanDirection::NONE;
        } else if (action == GLFW_PRESS) {
            g_should_pan_x = PanDirection::P;
	        if(!smooth_ctrl) g_camera.pan_x(ELAPSED, (int) g_should_pan_x);
        }
	} else if (key == GLFW_KEY_LEFT) { // roll counter
        if (action == GLFW_RELEASE && g_should_roll == RollDirection::COUNTER) {
            g_should_roll = RollDirection::NONE;
        } else if (action == GLFW_PRESS) {
            g_should_roll = RollDirection::COUNTER;
	        if(!smooth_ctrl) g_camera.roll(ELAPSED, (int) g_should_roll);
        }
	} else if (key == GLFW_KEY_RIGHT) { // roll clockwise
        if (action == GLFW_RELEASE && g_should_roll == RollDirection::CLOCK) {
            g_should_roll = RollDirection::NONE;
        } else if (action == GLFW_PRESS) {
            g_should_roll = RollDirection::CLOCK;
	        if(!smooth_ctrl) g_camera.roll(ELAPSED, (int) g_should_roll);
        }
	} else if (key == GLFW_KEY_DOWN) { // pan up
        if (action == GLFW_RELEASE && g_should_pan_y == PanDirection::P) {
            g_should_pan_y = PanDirection::NONE;
        } else if (action == GLFW_PRESS) {
            g_should_pan_y = PanDirection::P;
	        if(!smooth_ctrl) g_camera.pan_y(ELAPSED, (int) g_should_pan_y);
        }
	} else if (key == GLFW_KEY_UP) { // pan down
        if (action == GLFW_RELEASE && g_should_pan_y == PanDirection::N) {
            g_should_pan_y = PanDirection::NONE;
        } else if (action == GLFW_PRESS) {
            g_should_pan_y = PanDirection::N;
	        if(!smooth_ctrl) g_camera.pan_y(ELAPSED, (int) g_should_pan_y);
        }
	} else if (key == GLFW_KEY_C && action == GLFW_PRESS) {
		g_camera.change_mode();
	} else if (key == GLFW_KEY_MINUS && action == GLFW_PRESS) { // outer -
		if (tcs_out_deg > 1.0) tcs_out_deg -= 1.0;
	} else if (key == GLFW_KEY_EQUAL && action == GLFW_PRESS) { // outer +
		tcs_out_deg += 1.0;
	} else if (key == GLFW_KEY_COMMA && action == GLFW_PRESS) { // inner -
		if (tcs_in_deg > 1.0) tcs_in_deg -= 1.0;
	} else if (key == GLFW_KEY_PERIOD && action == GLFW_PRESS) { // inner +
		tcs_in_deg += 1.0;
	} else if (key == GLFW_KEY_O && mods == GLFW_MOD_CONTROL && action == GLFW_RELEASE) {
		enable_ocean = !enable_ocean;
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
	/*** model generation ************************************/

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
	std::vector<glm::vec4> ocean_vertices;
	std::vector<glm::uvec4> ocean_faces;
	getFloorQuad(ocean_vertices, ocean_faces);


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

	// Setup our VAO array.
	CHECK_GL_ERROR(glGenVertexArrays(kNumVaos, &g_array_objects[kGeometryVao]));

	// Switch to the VAO for Geometry.
	CHECK_GL_ERROR(glBindVertexArray(g_array_objects[kGeometryVao]));

	// Generate buffer objects
	CHECK_GL_ERROR(glGenBuffers(kNumVbos, &g_buffer_objects[kGeometryVao][0]));

	// Setup vertex data in a VBO.
	CHECK_GL_ERROR(glBindBuffer(GL_ARRAY_BUFFER, g_buffer_objects[kGeometryVao][kVertexBuffer]));
	// NOTE: We do not send anything right now, we just describe it to OpenGL.
	CHECK_GL_ERROR(glBufferData(GL_ARRAY_BUFFER,
		sizeof(float) * obj_vertices.size() * 4, obj_vertices.data(),
		GL_STATIC_DRAW));
	CHECK_GL_ERROR(glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0));
	CHECK_GL_ERROR(glEnableVertexAttribArray(0));

	// Setup element array buffer.
	CHECK_GL_ERROR(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_buffer_objects[kGeometryVao][kIndexBuffer]));
	CHECK_GL_ERROR(glBufferData(GL_ELEMENT_ARRAY_BUFFER,
		sizeof(uint32_t) * obj_faces.size() * 3,
		obj_faces.data(), GL_STATIC_DRAW));


	/*** Floor Program ***/

	CHECK_GL_ERROR(glGenVertexArrays(kNumVaos, &g_array_objects[kFloorVao]));
	// Switch to the VAO for Floor.
	CHECK_GL_ERROR(glBindVertexArray(g_array_objects[kFloorVao]));

	// Generate buffer objects
	CHECK_GL_ERROR(glGenBuffers(kNumVbos, &g_buffer_objects[kFloorVao][0]));

	// Setup vertex data in a VBO.
	CHECK_GL_ERROR(glBindBuffer(GL_ARRAY_BUFFER, g_buffer_objects[kFloorVao][kVertexBuffer]));
	// NOTE: We do not send anything right now, we just describe it to OpenGL.
	CHECK_GL_ERROR(glBufferData(GL_ARRAY_BUFFER, 
				sizeof(float) * floor_vertices.size() * 4, floor_vertices.data(),
				GL_STATIC_DRAW));
	CHECK_GL_ERROR(glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0));
	CHECK_GL_ERROR(glEnableVertexAttribArray(0));

	// Setup element array buffer.
	CHECK_GL_ERROR(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_buffer_objects[kFloorVao][kIndexBuffer]));
	CHECK_GL_ERROR(glBufferData(GL_ELEMENT_ARRAY_BUFFER, 
				sizeof(uint32_t) * floor_faces.size() * 3,
				floor_faces.data(), GL_STATIC_DRAW));


	/*** Ocean Program ***/

	CHECK_GL_ERROR(glGenVertexArrays(kNumVaos, &g_array_objects[kOceanVao]));
	// Switch to the VAO for Floor.
	CHECK_GL_ERROR(glBindVertexArray(g_array_objects[kOceanVao]));

	// Generate buffer objects
	CHECK_GL_ERROR(glGenBuffers(kNumVbos, &g_buffer_objects[kOceanVao][0]));

	// Setup vertex data in a VBO.
	CHECK_GL_ERROR(glBindBuffer(GL_ARRAY_BUFFER, g_buffer_objects[kOceanVao][kVertexBuffer]));
	// NOTE: We do not send anything right now, we just describe it to OpenGL.
	CHECK_GL_ERROR(glBufferData(GL_ARRAY_BUFFER, 
				sizeof(float) * ocean_vertices.size() * 4, ocean_vertices.data(),
				GL_STATIC_DRAW));
	CHECK_GL_ERROR(glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0));
	CHECK_GL_ERROR(glEnableVertexAttribArray(0));

	// Setup element array buffer.
	CHECK_GL_ERROR(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_buffer_objects[kOceanVao][kIndexBuffer]));
	CHECK_GL_ERROR(glBufferData(GL_ELEMENT_ARRAY_BUFFER, 
				sizeof(uint32_t) * ocean_faces.size() * 4,
				ocean_faces.data(), GL_STATIC_DRAW));



	/*********************************************************/
	/*** OpenGL: Shaders *************************************/

	 // Setup vertex shader.
	GLuint vertex_shader_id = 0;
	const char* vertex_source_pointer = vertex_shader;
	CHECK_GL_ERROR(vertex_shader_id = glCreateShader(GL_VERTEX_SHADER));
	CHECK_GL_ERROR(glShaderSource(vertex_shader_id, 1, &vertex_source_pointer, nullptr));
	glCompileShader(vertex_shader_id);
	CHECK_GL_SHADER_ERROR(vertex_shader_id);
	GLuint t_vertex_shader_id = 0;
	const char* t_vertex_source_pointer = t_vertex_shader;
	CHECK_GL_ERROR(t_vertex_shader_id = glCreateShader(GL_VERTEX_SHADER));
	CHECK_GL_ERROR(glShaderSource(t_vertex_shader_id, 1, &t_vertex_source_pointer, nullptr));
	glCompileShader(t_vertex_shader_id);
	CHECK_GL_SHADER_ERROR(t_vertex_shader_id);

	// Setup tessellation shaders for triangles.
	GLuint t_ctrl_shader_id = 0;
	const char* tcs_source_pointer = t_ctrl_shader;
	CHECK_GL_ERROR(t_ctrl_shader_id = glCreateShader(GL_TESS_CONTROL_SHADER));
	CHECK_GL_ERROR(glShaderSource(t_ctrl_shader_id, 1, &tcs_source_pointer, nullptr));
	glCompileShader(t_ctrl_shader_id);
	CHECK_GL_SHADER_ERROR(t_ctrl_shader_id);
	GLuint t_eval_shader_id = 0;
	const char* tes_source_pointer = t_eval_shader;
	CHECK_GL_ERROR(t_eval_shader_id = glCreateShader(GL_TESS_EVALUATION_SHADER));
	CHECK_GL_ERROR(glShaderSource(t_eval_shader_id, 1, &tes_source_pointer, nullptr));
	glCompileShader(t_eval_shader_id);
	CHECK_GL_SHADER_ERROR(t_eval_shader_id);

	// Setup tessellation shaders for quads.
	GLuint quad_t_ctrl_shader_id = 0;
	const char* quad_tcs_source_pointer = quad_t_ctrl_shader;
	CHECK_GL_ERROR(quad_t_ctrl_shader_id = glCreateShader(GL_TESS_CONTROL_SHADER));
	CHECK_GL_ERROR(glShaderSource(quad_t_ctrl_shader_id, 1, &quad_tcs_source_pointer, nullptr));
	glCompileShader(quad_t_ctrl_shader_id);
	CHECK_GL_SHADER_ERROR(quad_t_ctrl_shader_id);
	GLuint quad_t_eval_shader_id = 0;
	const char* quad_tes_source_pointer = quad_t_eval_shader;
	CHECK_GL_ERROR(quad_t_eval_shader_id = glCreateShader(GL_TESS_EVALUATION_SHADER));
	CHECK_GL_ERROR(glShaderSource(quad_t_eval_shader_id, 1, &quad_tes_source_pointer, nullptr));
	glCompileShader(quad_t_eval_shader_id);
	CHECK_GL_SHADER_ERROR(quad_t_eval_shader_id);

	// Setup geometry shader.
	GLuint geometry_shader_id = 0;
	const char* geometry_source_pointer = geometry_shader;
	CHECK_GL_ERROR(geometry_shader_id = glCreateShader(GL_GEOMETRY_SHADER));
	CHECK_GL_ERROR(glShaderSource(geometry_shader_id, 1, &geometry_source_pointer, nullptr));
	glCompileShader(geometry_shader_id);
	CHECK_GL_SHADER_ERROR(geometry_shader_id);

	// Setup fragment shader.
	GLuint fragment_shader_id = 0;
	const char* fragment_source_pointer = fragment_shader;
	CHECK_GL_ERROR(fragment_shader_id = glCreateShader(GL_FRAGMENT_SHADER));
	CHECK_GL_ERROR(glShaderSource(fragment_shader_id, 1, &fragment_source_pointer, nullptr));
	glCompileShader(fragment_shader_id);
	CHECK_GL_SHADER_ERROR(fragment_shader_id);

	// Setup fragment shader for the floor
	GLuint floor_fragment_shader_id = 0;
	const char* floor_fragment_source_pointer = floor_fragment_shader;
	CHECK_GL_ERROR(floor_fragment_shader_id = glCreateShader(GL_FRAGMENT_SHADER));
	CHECK_GL_ERROR(glShaderSource(floor_fragment_shader_id, 1, &floor_fragment_source_pointer, nullptr));
	glCompileShader(floor_fragment_shader_id);
	CHECK_GL_SHADER_ERROR(floor_fragment_shader_id);



	/*********************************************************/
	/*** OpenGL: Programs ************************************/

	/*** Geometry Program ***/

	// Let's create our program.
	GLuint program_id = 0;
	CHECK_GL_ERROR(program_id = glCreateProgram());
	CHECK_GL_ERROR(glAttachShader(program_id, vertex_shader_id));
	CHECK_GL_ERROR(glAttachShader(program_id, fragment_shader_id));
	CHECK_GL_ERROR(glAttachShader(program_id, geometry_shader_id));

	// Bind attributes.
	CHECK_GL_ERROR(glBindAttribLocation(program_id, 0, "vertex_position"));
	CHECK_GL_ERROR(glBindFragDataLocation(program_id, 0, "fragment_color"));
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
		glGetUniformLocation(program_id, "light_position"));


	/*** Floor Program ***/

	// create program
	GLuint floor_program_id = 0;
	CHECK_GL_ERROR(floor_program_id = glCreateProgram());
	CHECK_GL_ERROR(glAttachShader(floor_program_id, t_vertex_shader_id));
	CHECK_GL_ERROR(glAttachShader(floor_program_id, t_ctrl_shader_id));
	CHECK_GL_ERROR(glAttachShader(floor_program_id, t_eval_shader_id));
	CHECK_GL_ERROR(glAttachShader(floor_program_id, geometry_shader_id));
	CHECK_GL_ERROR(glAttachShader(floor_program_id, floor_fragment_shader_id));

	// bind attributes
	CHECK_GL_ERROR(glBindAttribLocation(floor_program_id, 0, "vertex_position"));
	CHECK_GL_ERROR(glBindFragDataLocation(floor_program_id, 0, "fragment_color"));
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
			glGetUniformLocation(floor_program_id, "light_position"));
	GLint tcs_in_deg_location = 0;
	CHECK_GL_ERROR(tcs_in_deg_location =
			glGetUniformLocation(floor_program_id, "tcs_in_deg"));
	GLint tcs_out_deg_location = 0;
	CHECK_GL_ERROR(tcs_out_deg_location =
			glGetUniformLocation(floor_program_id, "tcs_out_deg"));


	/*** Ocean Program ***/

	// create program
	GLuint ocean_program_id = 0;
	CHECK_GL_ERROR(ocean_program_id = glCreateProgram());
	CHECK_GL_ERROR(glAttachShader(ocean_program_id, t_vertex_shader_id));
	CHECK_GL_ERROR(glAttachShader(ocean_program_id, quad_t_ctrl_shader_id));
	CHECK_GL_ERROR(glAttachShader(ocean_program_id, quad_t_eval_shader_id));
	CHECK_GL_ERROR(glAttachShader(ocean_program_id, geometry_shader_id));
	CHECK_GL_ERROR(glAttachShader(ocean_program_id, floor_fragment_shader_id));

	// bind attributes
	CHECK_GL_ERROR(glBindAttribLocation(ocean_program_id, 0, "vertex_position"));
	CHECK_GL_ERROR(glBindFragDataLocation(ocean_program_id, 0, "fragment_color"));
	glLinkProgram(ocean_program_id);
	CHECK_GL_PROGRAM_ERROR(ocean_program_id);

	// unifrom locations
	GLint ocean_projection_matrix_location = 0;
	CHECK_GL_ERROR(ocean_projection_matrix_location =
			glGetUniformLocation(ocean_program_id, "projection"));
	GLint ocean_view_matrix_location = 0;
	CHECK_GL_ERROR(ocean_view_matrix_location =
			glGetUniformLocation(ocean_program_id, "view"));
	GLint ocean_light_position_location = 0;
	CHECK_GL_ERROR(ocean_light_position_location =
			glGetUniformLocation(ocean_program_id, "light_position"));
	GLint ocean_tcs_in_deg_location = 0;
	CHECK_GL_ERROR(ocean_tcs_in_deg_location =
			glGetUniformLocation(ocean_program_id, "tcs_in_deg"));
	GLint ocean_tcs_out_deg_location = 0;
	CHECK_GL_ERROR(ocean_tcs_out_deg_location =
			glGetUniformLocation(ocean_program_id, "tcs_out_deg"));



	/*********************************************************/
	/*** OpenGL: Uniforms ************************************/
    int level = 0;
	glm::vec4 light_position = glm::vec4(10.0f, 10.0f, 10.0f, 1.0f);
	float aspect = 0.0f;
	float theta = 0.0f;
    g_lt = std::chrono::system_clock::now();
	while (!glfwWindowShouldClose(window)) {

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
		CHECK_GL_ERROR(glBindVertexArray(g_array_objects[kGeometryVao]));

		if (g_menger && g_menger->is_dirty()) {

            g_menger->generate_geometry(obj_vertices, obj_faces);
			g_menger->set_clean();

			CHECK_GL_ERROR(glBindBuffer(GL_ARRAY_BUFFER, g_buffer_objects[kGeometryVao][kVertexBuffer]));
			CHECK_GL_ERROR(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_buffer_objects[kGeometryVao][kIndexBuffer]));
            // FIXME: Upload your vertex data here.

            // Setup vertex data in a VBO.
            // CHECK_GL_ERROR(glBindBuffer(GL_ARRAY_BUFFER, g_buffer_objects[kGeometryVao][kVertexBuffer]));
            CHECK_GL_ERROR(glBufferData(GL_ARRAY_BUFFER, sizeof(float) * obj_vertices.size() * 4, obj_vertices.data(), GL_STATIC_DRAW));

            // Setup element array buffer.
            // CHECK_GL_ERROR(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_buffer_objects[kGeometryVao][kIndexBuffer]));
            CHECK_GL_ERROR(glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint32_t) * obj_faces.size() * 3, obj_faces.data(), GL_STATIC_DRAW));
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

		/*** Geometry Program ***/

		// Use our program.
		CHECK_GL_ERROR(glUseProgram(program_id));
		CHECK_GL_ERROR(glBindVertexArray(g_array_objects[kGeometryVao]));

		// Pass uniforms in.
		CHECK_GL_ERROR(glUniformMatrix4fv(projection_matrix_location, 1, GL_FALSE,
			&projection_matrix[0][0]));
		CHECK_GL_ERROR(glUniformMatrix4fv(view_matrix_location, 1, GL_FALSE,
			&view_matrix[0][0]));
		CHECK_GL_ERROR(glUniform4fv(light_position_location, 1, &light_position[0]));

		// Draw our triangles.
		CHECK_GL_ERROR(glDrawElements(GL_TRIANGLES, obj_faces.size() * 3, GL_UNSIGNED_INT, 0));


		/*** Floor Program ***/

		if(!enable_ocean) {
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

			// Render floor
			CHECK_GL_ERROR(glPatchParameteri(GL_PATCH_VERTICES, 3));
			CHECK_GL_ERROR(glDrawElements(GL_PATCHES, floor_faces.size() * 3, GL_UNSIGNED_INT, 0));
			// CHECK_GL_ERROR(glDrawElements(GL_TRIANGLES, floor_faces.size() * 3, GL_UNSIGNED_INT, 0));
			// CHECK_GL_ERROR(glDrawArrays(GL_PATCHES, 0, floor_faces.size() * 3)); // ???	
		}
		else {
			// set program + vao
			CHECK_GL_ERROR(glUseProgram(ocean_program_id));
			CHECK_GL_ERROR(glBindVertexArray(g_array_objects[kOceanVao]));

			// pass uniforms
			CHECK_GL_ERROR(glUniformMatrix4fv(ocean_projection_matrix_location, 1, GL_FALSE,
				&projection_matrix[0][0]));
			CHECK_GL_ERROR(glUniformMatrix4fv(ocean_view_matrix_location, 1, GL_FALSE,
				&view_matrix[0][0]));
			CHECK_GL_ERROR(glUniform4fv(ocean_light_position_location, 1, &light_position[0]));
			CHECK_GL_ERROR(glUniform1f(ocean_tcs_in_deg_location, tcs_in_deg));
			CHECK_GL_ERROR(glUniform1f(ocean_tcs_out_deg_location, tcs_out_deg));

			// Render floor
			CHECK_GL_ERROR(glPatchParameteri(GL_PATCH_VERTICES, 4));
			CHECK_GL_ERROR(glDrawElements(GL_PATCHES, ocean_faces.size() * 4, GL_UNSIGNED_INT, 0));
			// CHECK_GL_ERROR(glDrawElements(GL_TRIANGLES, floor_faces.size() * 3, GL_UNSIGNED_INT, 0));
			// CHECK_GL_ERROR(glDrawArrays(GL_PATCHES, 0, floor_faces.size() * 3)); // ???	
		}


		/*********************************************************/
		/*** Next Iteration **************************************/

		// get interaction
		glfwPollEvents();

		// swap buffer
		glfwSwapBuffers(window);

		// smooth 
		if (smooth_ctrl) {
			// time delta smoothing
	        auto ct = std::chrono::system_clock::now();
	        double elapsed = (ct - g_lt).count();
	        g_lt = ct;
	        elapsed = 0.1;
	        if ((int) g_should_move) {
	            g_camera.move(elapsed, (int) g_should_move);
	        }
	        if ((int) g_should_roll) {
	            g_camera.roll(elapsed, (int) g_should_roll);
	        }
	        if ((int) g_should_pan_x) {
	            g_camera.pan_x(elapsed, (int) g_should_pan_x);
	        }
	        if ((int) g_should_pan_y) {
	            g_camera.pan_y(elapsed, (int) g_should_pan_y);
	        }
		}
	}

	/*********************************************************/
	/*** OpenGL: Clean up ************************************/

	glfwDestroyWindow(window);
	glfwTerminate();
	exit(EXIT_SUCCESS);
}
