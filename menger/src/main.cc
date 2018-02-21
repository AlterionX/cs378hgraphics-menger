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

int window_width = 800, window_height = 600;

// VBO and VAO descriptors.
enum { kVertexBuffer, kIndexBuffer, kNumVbos };

// These are our VAOs.
enum { kGeometryVao, kFloorVao, kNumVaos };

GLuint g_array_objects[kNumVaos];  // This will store the VAO descriptors.
GLuint g_buffer_objects[kNumVaos][kNumVbos];  // These will store VBO descriptors.

// C++ 11 String Literal
// See http://en.cppreference.com/w/cpp/language/string_literal
const char* vertex_shader =
R"zzz(#version 330 core
in vec4 vertex_position;
uniform mat4 view;
uniform vec4 light_position;
out vec4 vs_light_direction;
out vec4 g_old;
void main()
{
    g_old = vertex_position;
	gl_Position = view * vertex_position;
	vs_light_direction = -gl_Position + view * light_position;
}
)zzz";

const char* geometry_shader =
R"zzz(#version 330 core
layout (triangles) in;
layout (triangle_strip, max_vertices = 3) out;
uniform mat4 projection;
in vec4 vs_light_direction[];
in vec4 g_old[];
flat out vec4 normal;
out vec4 light_direction;
void main()
{
	int n = 0;
    vec3 p0 = g_old[0].xyz;
    vec3 p1 = g_old[1].xyz;
    vec3 p2 = g_old[2].xyz;
    normal = vec4(normalize(cross(p1 - p0, p2 - p0)), 0.0);
	for (n = 0; n < gl_in.length(); n++) {
		light_direction = vs_light_direction[n];
		gl_Position = projection * gl_in[n].gl_Position;
		EmitVertex();
	}
    EndPrimitive();
}
)zzz";

const char* fragment_shader =
R"zzz(#version 330 core
flat in vec4 normal;
in vec4 light_direction;
out vec4 fragment_color;
void main()
{
	vec4 color = clamp(vec4(vec3(normal * normal), 1.0), 0.0, 1.0);
	float dot_nl = dot(normalize(light_direction), normalize(normal));
	dot_nl = clamp(dot_nl, 0.0, 1.0);
	fragment_color = clamp(dot_nl * color, 0.0, 1.0);
}
)zzz";

// FIXME: Implement shader effects with an alternative shader.
const char* floor_fragment_shader =
R"zzz(#version 330 core
in vec4 normal;
in vec4 light_direction;
in vec4 world_position;
out vec4 fragment_color;
void main()
{
	fragment_color = vec4(0.0, 0.0, 0.0, 1.0);
}
)zzz";

void CreateTriangle(std::vector<glm::vec4>& vertices, std::vector<glm::uvec3>& indices) {
	vertices.push_back(glm::vec4(-0.5f, -0.5f, -0.5f, 1.0f));
	vertices.push_back(glm::vec4(0.5f, -0.5f, -0.5f, 1.0f));
	vertices.push_back(glm::vec4(0.0f, 0.5f, -0.5f, 1.0f));
	indices.push_back(glm::uvec3(0, 1, 2));
}

// FIXME: Save geometry to OBJ file
void SaveObj(const std::string& file,
	const std::vector<glm::vec4>& vertices,
	const std::vector<glm::uvec3>& indices) {}

void
ErrorCallback(int error, const char* description) {
	std::cerr << "GLFW Error: " << description << "\n";
}

std::shared_ptr<Menger> g_menger;
Camera g_camera;

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
		// FIXME: save geometry to OBJ
	} else if (key == GLFW_KEY_W) { // move forwards
        if (action == GLFW_RELEASE && g_should_move == MovementDirection::FORWARD) {
            g_should_move = MovementDirection::NONE;
        } else if (action == GLFW_PRESS) {
    		g_should_move = MovementDirection::FORWARD;
        }
	} else if (key == GLFW_KEY_S) { // move backwards
        if (action == GLFW_RELEASE && g_should_move == MovementDirection::BACKWARD) {
            g_should_move = MovementDirection::NONE;
        } else if (action == GLFW_PRESS) {
            g_should_move = MovementDirection::BACKWARD;
        }
	} else if (key == GLFW_KEY_A) { // pan left
        if (action == GLFW_RELEASE && g_should_pan_x == PanDirection::N) {
            g_should_pan_x = PanDirection::NONE;
        } else if (action == GLFW_PRESS) {
            g_should_pan_x = PanDirection::N;
        }
	} else if (key == GLFW_KEY_D) { // pan right
        if (action == GLFW_RELEASE && g_should_pan_x == PanDirection::P) {
            g_should_pan_x = PanDirection::NONE;
        } else if (action == GLFW_PRESS) {
            g_should_pan_x = PanDirection::P;
        }
	} else if (key == GLFW_KEY_LEFT) { // roll counter
        if (action == GLFW_RELEASE && g_should_roll == RollDirection::COUNTER) {
            g_should_roll = RollDirection::NONE;
        } else if (action == GLFW_PRESS) {
            g_should_roll = RollDirection::COUNTER;
        }
	} else if (key == GLFW_KEY_RIGHT) { // roll clockwise
        if (action == GLFW_RELEASE && g_should_roll == RollDirection::CLOCK) {
            g_should_roll = RollDirection::NONE;
        } else if (action == GLFW_PRESS) {
            g_should_roll = RollDirection::CLOCK;
        }
	} else if (key == GLFW_KEY_DOWN) { // pan up
        if (action == GLFW_RELEASE && g_should_pan_y == PanDirection::P) {
            g_should_pan_y = PanDirection::NONE;
        } else if (action == GLFW_PRESS) {
            g_should_pan_y = PanDirection::P;
        }
	} else if (key == GLFW_KEY_UP) { // pan down
        if (action == GLFW_RELEASE && g_should_pan_y == PanDirection::N) {
            g_should_pan_y = PanDirection::NONE;
        } else if (action == GLFW_PRESS) {
            g_should_pan_y = PanDirection::N;
        }
	} else if (key == GLFW_KEY_C && action != GLFW_RELEASE) {
		g_camera.toggle_mode();
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
	std::string window_title = "Menger";
	if (!glfwInit()) exit(EXIT_FAILURE);
	g_menger = std::make_shared<Menger>();
	glfwSetErrorCallback(ErrorCallback);

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

	std::vector<glm::vec4> obj_vertices;
	std::vector<glm::uvec3> obj_faces;

	//FIXME: Create the geometry from a Menger object.
    CreateTriangle(obj_vertices, obj_faces);
	g_menger->set_nesting_level(1);

	glm::vec4 min_bounds = glm::vec4(std::numeric_limits<float>::max());
	glm::vec4 max_bounds = glm::vec4(-std::numeric_limits<float>::max());
	for (const auto& vert : obj_vertices) {
		min_bounds = glm::min(vert, min_bounds);
		max_bounds = glm::max(vert, max_bounds);
	}
	std::cout << "min_bounds = " << glm::to_string(min_bounds) << "\n";
	std::cout << "max_bounds = " << glm::to_string(max_bounds) << "\n";

	// Setup our VAO array.
	CHECK_GL_ERROR(glGenVertexArrays(kNumVaos, &g_array_objects[0]));

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

	/*
	 * By far, the geometry is loaded into g_buffer_objects[kGeometryVao][*].
	 * These buffers are binded to g_array_objects[kGeometryVao]
	 */

	 // FIXME: load the floor into g_buffer_objects[kFloorVao][*],
	 //        and bind these VBO to g_array_objects[kFloorVao]

	 // Setup vertex shader.
	GLuint vertex_shader_id = 0;
	const char* vertex_source_pointer = vertex_shader;
	CHECK_GL_ERROR(vertex_shader_id = glCreateShader(GL_VERTEX_SHADER));
	CHECK_GL_ERROR(glShaderSource(vertex_shader_id, 1, &vertex_source_pointer, nullptr));
	glCompileShader(vertex_shader_id);
	CHECK_GL_SHADER_ERROR(vertex_shader_id);

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

	// Setup fragment shader for the floor
	GLuint floor_fragment_shader_id = 0;
	const char* floor_fragment_source_pointer = floor_fragment_shader;
	CHECK_GL_ERROR(floor_fragment_shader_id = glCreateShader(GL_FRAGMENT_SHADER));
	CHECK_GL_ERROR(glShaderSource(floor_fragment_shader_id, 1,
		&floor_fragment_source_pointer, nullptr));
	glCompileShader(floor_fragment_shader_id);
	CHECK_GL_SHADER_ERROR(floor_fragment_shader_id);

	// FIXME: Setup another program for the floor, and get its locations.
	// Note: you can reuse the vertex and geometry shader objects
	GLuint floor_program_id = 0;
	GLint floor_projection_matrix_location = 0;
	GLint floor_view_matrix_location = 0;
	GLint floor_light_position_location = 0;

    int level = 0;

	glm::vec4 light_position = glm::vec4(10.0f, 10.0f, 10.0f, 1.0f);
	float aspect = 0.0f;
	float theta = 0.0f;
    g_lt = std::chrono::system_clock::now();
	while (!glfwWindowShouldClose(window)) {
		// Setup some basic window stuff.
		glfwGetFramebufferSize(window, &window_width, &window_height);
		glViewport(0, 0, window_width, window_height);
		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		glEnable(GL_DEPTH_TEST);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glDepthFunc(GL_LESS);

		// Switch to the Geometry VAO.
		CHECK_GL_ERROR(glBindVertexArray(g_array_objects[kGeometryVao]));

		if (g_menger && g_menger->is_dirty()) {
            g_menger->generate_geometry(obj_vertices, obj_faces);
			g_menger->set_clean();

            // FIXME: Upload your vertex data here.

            // Setup vertex data in a VBO.
            // CHECK_GL_ERROR(glBindBuffer(GL_ARRAY_BUFFER, g_buffer_objects[kGeometryVao][kVertexBuffer]));
            CHECK_GL_ERROR(glBufferData(GL_ARRAY_BUFFER, sizeof(float) * obj_vertices.size() * 4, obj_vertices.data(), GL_STATIC_DRAW));

            // Setup element array buffer.
            // CHECK_GL_ERROR(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_buffer_objects[kGeometryVao][kIndexBuffer]));
            CHECK_GL_ERROR(glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint32_t) * obj_faces.size() * 3, obj_faces.data(), GL_STATIC_DRAW));
		}

		// Compute the projection matrix.
		aspect = static_cast<float>(window_width) / window_height;
		glm::mat4 projection_matrix =
			glm::perspective(glm::radians(45.0f), aspect, 0.0001f, 1000.0f);

		// Compute the view matrix
		// FIXME: change eye and center through mouse/keyboard events.
		glm::mat4 view_matrix = g_camera.get_view_matrix();

		// Use our program.
		CHECK_GL_ERROR(glUseProgram(program_id));

		// Pass uniforms in.
		CHECK_GL_ERROR(glUniformMatrix4fv(projection_matrix_location, 1, GL_FALSE,
			&projection_matrix[0][0]));
		CHECK_GL_ERROR(glUniformMatrix4fv(view_matrix_location, 1, GL_FALSE,
			&view_matrix[0][0]));
		CHECK_GL_ERROR(glUniform4fv(light_position_location, 1, &light_position[0]));

		// Draw our triangles.
		CHECK_GL_ERROR(glDrawElements(GL_TRIANGLES, obj_faces.size() * 3, GL_UNSIGNED_INT, 0));

		// FIXME: Render the floor
		// Note: What you need to do is
		// 	1. Switch VAO
		// 	2. Switch Program
		// 	3. Pass Uniforms
		// 	4. Call glDrawElements, since input geometry is
		// 	indicated by VAO.

		// Poll and swap.
		glfwPollEvents();

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

		glfwSwapBuffers(window);
	}
	glfwDestroyWindow(window);
	glfwTerminate();
	exit(EXIT_SUCCESS);
}
