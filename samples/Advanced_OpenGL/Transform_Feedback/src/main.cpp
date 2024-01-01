/*
	SRC-CODE FOR
	- Transform Feedback
	
	Source: https://open.gl/feedback
*/

// MODE 0 - SQRT Root Example
//      1 - Geometry shader
//      2 - Points Hover around mouse
#define MODE 2

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <chrono>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "stb_image.h"

#include "modules/shader_m.h"
#include "modules/camera.h"
#include "modules/model.h"
#include "modules/filesystem.h"
#include "modules/light.h"
#include "modules/material.h"

#include <iostream>

// callbacks
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
// current Mouse position 
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);
/* load texture */
unsigned int loadTexture(const char* path, GLenum wrap);

GLuint createTransformFeedbackShader(std::vector<const GLchar*> feedbackVaryings, const char* vertexPath, const char* fragmentPath = nullptr, const char* geometryPath=nullptr);

void icon(GLFWwindow* window);

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 800;
int sizex = SCR_WIDTH;
int sizey = SCR_HEIGHT;
// camera
Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
float lastX = SCR_WIDTH / 2.0;
float lastY = SCR_HEIGHT / 2.0;
bool firstMouse = true;
float radius = 0.75; // attraction radius
// timing 
float deltaTime = 0.0f; // time between current frame and last frame
float lastFrame = 0.0f; // time of last frame

#define GEOM_VERT_COUNT 6

int main()
{
	// glfw: initialize and configure
	// ------------------------------
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // uncomment this statement to fix compilation on OS X
#endif

// glfw window creation
// --------------------
	GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Advanced GL", NULL, NULL);
	/* creates and configures default framebuffer */

	if (window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetScrollCallback(window, scroll_callback);

	//tell GLFW to capture our mouse
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

	// glad: load all OpenGL function pointers
	// ---------------------------------------
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
		return -1;
	}

	icon(window);

	// configure global opengl state
	// -----------------------------

	// specify transform feedback variables
	std::vector<const GLchar*> feedbackVaryings;

	// create transform feedback buffer
#if MODE == 0
	feedbackVaryings.push_back("outValue");
	GLuint program = createTransformFeedbackShader(feedbackVaryings,FileSystem::getSamplePath("shader/sqrtRootTF.vert").c_str());
#elif MODE == 1
	feedbackVaryings.push_back("outValue");
	GLuint program = createTransformFeedbackShader(feedbackVaryings, FileSystem::getSamplePath("shader/geometryShader.vert").c_str(), nullptr, FileSystem::getSamplePath("shader/geometryShader.geom").c_str());
#elif MODE == 2
	feedbackVaryings.push_back("outPosition");
	feedbackVaryings.push_back("outVelocity");
	GLuint program = createTransformFeedbackShader(feedbackVaryings, FileSystem::getSamplePath("shader/renderPipeline.vert").c_str(), FileSystem::getSamplePath("shader/renderPipeline.frag").c_str());
#endif
	// use shader
	glUseProgram(program);
#if MODE == 2
	GLint uniTime = glGetUniformLocation(program, "time");
	GLint uniMousePos = glGetUniformLocation(program, "mousePos");
	GLint uniRadius = glGetUniformLocation(program, "radius");
#endif
	// Create VAO
	GLuint vao;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

#if MODE == 0 || MODE == 1
	// Create input VBO and vertex format
	vector<GLfloat> vertexData = { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f }; // some input data for the vertex shader

	GLuint vbo;
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*vertexData.size(), vertexData.data(), GL_STATIC_DRAW);

	// set the vertex pointers to the data
	GLint inputAttrib = glGetAttribLocation(program, "inValue");
	glEnableVertexAttribArray(inputAttrib);
	glVertexAttribPointer(inputAttrib, 1, GL_FLOAT, GL_FALSE, 0, 0);

#elif MODE == 2
	// Create input VBO and vertex format
	struct Vertex {
		glm::vec2 pos;
		glm::vec2 vel;
		glm::vec2 origPos;
	};

	vector<Vertex> vertices;

	// Set original and initial positions
	for (int y = 0; y < 100; y++) {
		for (int x = 0; x < 100; x++) {
			Vertex v;
			v.pos = glm::vec2(0.2f/10.0f * x-1.0f, 0.2f / 10.0f * y-1.0f);
			v.vel = glm::vec2(0.0f, 0.0f);
			v.origPos = v.pos;

			vertices.push_back(v);
		}
	}

	GLuint vbo;
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex)*vertices.size(), vertices.data(), GL_STREAM_DRAW);

	// Vertex format: 6 floats per vertex:
	// pos.x  pox.y  vel.x  vel.y  origPos.x  origPos.y

	GLint posAttrib = glGetAttribLocation(program, "position");
	glEnableVertexAttribArray(posAttrib);
	glVertexAttribPointer(posAttrib, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), 0);

	GLint velAttrib = glGetAttribLocation(program, "velocity");
	glEnableVertexAttribArray(velAttrib); 
	glVertexAttribPointer(velAttrib, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(2 * sizeof(GLfloat)));

	GLint origPosAttrib = glGetAttribLocation(program, "originalPos");
	glEnableVertexAttribArray(origPosAttrib);
	glVertexAttribPointer(origPosAttrib, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(4 * sizeof(GLfloat)));
#endif
	
	// Create transform feedback buffer (VBO) to hold the output data of the 
	GLuint tbo;
	glGenBuffers(1, &tbo);
	glBindBuffer(GL_ARRAY_BUFFER, tbo);

	// NOTE: size of the buffer will be the amount of input data
	//       Data itself will be uninitialized (nullptr)
	//       GL_STAT_READ - OpenGL should write to this buffer
	//                      application shoudl readm from buffer
#if MODE == 0
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)* vertexData.size(), nullptr, GL_STATIC_READ);
#elif MODE == 1
	// caputre 3 times the amount due to the geometry shader
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * vertexData.size()* GEOM_VERT_COUNT, nullptr, GL_STATIC_READ);
#elif MODE == 2
	// reserve storage for position and velocities
	GLuint storage = vertices.size() * 4;
	glBufferData(GL_ARRAY_BUFFER, storage * sizeof(GLfloat), nullptr, GL_STATIC_READ);

	glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, tbo);
	vector<GLfloat>feedback;
	feedback.resize(storage);

#endif
	// START RENDER/COMPUTATION PROCESS
	// Perform feedback transform
	// --------------------------------
#if MODE == 0 || MODE == 1	
	// disable rasterize process
	// don't draw anything
	glEnable(GL_RASTERIZER_DISCARD);

	// Bindbuffer base to write into the Transform feedback buffer
	// void glBindBufferBase(GLenum target, GLuint index, GLuint buffer);
	// target: 
	//       GL_ATOMIC_COUNTER_BUFFER
	//       GL_TRANSFORM_FEEDBACK_BUFFER
	//       GL_UNIFORM_BUFFER 
	//       GL_SHADER_STORAGE_BUFFER
	// index: index of the output variable (in this case 0 as there's only one variable
	// buffer: buffer to bind (in this case the VBO setup as Transform Feedback Buffer)
	glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, tbo);

	// keep track of primitives written
	GLuint query, query2, query3;
	glGenQueries(1, &query);
	glGenQueries(1, &query2);
	glGenQueries(1, &query3);
	glBeginQuery(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN, query);
	glBeginQuery(GL_PRIMITIVES_GENERATED, query2);
	glBeginQuery(GL_TIME_ELAPSED, query3);
	// enter transform feedback mode 
	// define expected output, it must match the type of the output shader e.g. vertex/geom
	// 3 Options: 
	//       GL_POINTS    : GL_POINTS
	//       GL_LINES     : GL_LINES, GL_LINE_LOOP, GL_LINE_STRIP, GL_LINES_ADJACENCY, 
	//                      GL_LINE_STRIP_ADJACENCY
	//       GL_TRIANGLES : GL_TRIANGLES, GL_TRIANGLE_STRIP, GL_TRIANGLE_FAN, GL_TRIANGLES_ADJACENCY, 
	//                      GL_TRIANGLE_STRIP_ADJACENCY
#if MODE == 0
	glBeginTransformFeedback(GL_POINTS);
#elif MODE == 1
	glBeginTransformFeedback(GL_TRIANGLES);
#endif
	// NOTE: having just one vertex Shader, the primitive must match the one being drawn

	// invoce shader for n points / n numbers (in this case the amount of data in the vector)
	glDrawArrays(GL_POINTS, 0, vertexData.size());
	// end transform feedback mode
	glEndTransformFeedback();

	// stop recording of primitives written
	glEndQuery(GL_TIME_ELAPSED);
	glEndQuery(GL_PRIMITIVES_GENERATED);
	glEndQuery(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN);
	// enable rasterizer (by disabling the discard)
	glDisable(GL_RASTERIZER_DISCARD);

	// make sure render operation has finished before trying to access the results
	glFlush();

	// retrieve the result of written primitives
	// GL_PRIMITIVES_GENERATED: Records the number of primitives sent to a particular 
	//                          Geometry Shader output stream (or by stream 0 if no GS is active)
	//                          by the scoped drawing commands. 
	// GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN: Records the number of primitives written by a
	//                          Geometry Shader stream to a Transform Feedback object(or by
	//                          stream 0 if no GS is active) by the scoped drawing commands.
	// GL_TIME_ELAPSED​: Records the time that it takes for the GPU to execute all of the
	//                  scoped commands.The timer starts when all commands before the scope
	//                  have completed, and the timer ends when the last scoped command has
	//                  completed. (in nano seconds)
	GLuint primitives;
	glGetQueryObjectuiv(query, GL_QUERY_RESULT, &primitives);
	GLuint primitivesGen;
	glGetQueryObjectuiv(query2, GL_QUERY_RESULT, &primitivesGen);
	GLuint64 timeElapsed;
	glGetQueryObjectui64v(query3, GL_QUERY_RESULT, &timeElapsed);
	// this is the amount of primitives written (not vertices)
	std::cout << primitives << " primitives written!" << std::endl;
	std::cout << primitivesGen << " primitives generated!" << std::endl;
	double elapsed_time_in_milliseconds = static_cast<double>(timeElapsed) / 1000000.0;

	std::cout << elapsed_time_in_milliseconds << "ms time elapsed!" << std::endl;

	// Fetch and print results
	vector<GLfloat> feedback;
#if MODE == 0
	feedback.resize(vertexData.size());
#elif MODE ==1 
	// capture the amount of data created by the geometry shader
	feedback.resize(vertexData.size()* GEOM_VERT_COUNT);
#endif
	// copy data from buffer back to array
	glGetBufferSubData(GL_TRANSFORM_FEEDBACK_BUFFER, 0,sizeof(GLfloat)*feedback.size(), feedback.data());

	// output data
	for(GLfloat value : feedback) {
		std::cout << value << " ";
	}
	std::cout << std::endl;
#elif MODE == 2
	glBindBuffer(GL_ARRAY_BUFFER, vbo);

	glPointSize(5.0f);

	lastFrame = static_cast<float>(glfwGetTime());

	// render loop
	// -----------
	while (!glfwWindowShouldClose(window))
	{
		// Calculate delta time
		auto currentFrame = static_cast<float>(glfwGetTime());
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;
		glUniform1f(uniTime, deltaTime);

		// Check and call events
		processInput(window);

		// Clear the screen to black
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		// Update mouse position
		double xpos, ypos;
		glfwGetCursorPos(window, &xpos, &ypos);
		glUniform2f(uniMousePos, xpos / ((float)sizex/2.0f) - 1, -ypos / ((float)sizey/2.0f) + 1);
		glUniform1f(uniRadius, radius);

		// Perform feedback transform and draw vertices
		glBeginTransformFeedback(GL_POINTS);
		glDrawArrays(GL_POINTS, 0, vertices.size());
		glEndTransformFeedback();

		// glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
		// -------------------------------------------------------------------------------
		glfwSwapBuffers(window);
		glfwPollEvents();

		// Update vertices' position and velocity using transform feedback
		glGetBufferSubData(GL_TRANSFORM_FEEDBACK_BUFFER, 0, sizeof(GLfloat)*feedback.size(), feedback.data());

		for (int i = 0; i < vertices.size(); i++) {
			vertices[i].pos = glm::vec2(feedback[4 * i], feedback[4 * i + 1]);
			vertices[i].vel = glm::vec2(feedback[4 * i + 2], feedback[4 * i + 3]);
		}

		// glBufferData() would reallocate the whole vertex data buffer, which is unnecessary here.
		// glBufferSubData() is used instead - it updates an existing buffer.
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(Vertex)*vertices.size(), vertices.data());
	}
#endif
	


	// glfw: terminate, clearing all previously allocated GLFW resources.
	// ------------------------------------------------------------------
	glDeleteProgram(program);

	glDeleteBuffers(1, &tbo);
	glDeleteBuffers(1, &vbo);

	glDeleteVertexArrays(1, &vao);

	glfwTerminate();
	return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow* window)
{

	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);

	float cameraSpeed = 2.5f * deltaTime; // adjust accordingly

	if (glfwGetKey(window, GLFW_KEY_F2) == GLFW_PRESS)
	{
		const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
		glfwSetWindowMonitor(window, glfwGetPrimaryMonitor(), 0, 0, mode->width, mode->height, mode->refreshRate);
	}

}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
	//Rotation 
	if (glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS &&
		glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
		if (firstMouse)
		{
			lastX = xpos;
			lastY = ypos;
			firstMouse = false;
		}

		float xoffset = xpos - lastX;
		float yoffset = lastY - ypos;
		// reversed since y-coordinates range from bottom to top
		lastX = xpos;
		lastY = ypos;

		camera.rotate(xoffset, yoffset);
	}

	//Moving 
	if (glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS &&
		glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS) {
		if (firstMouse)
		{
			lastX = xpos;
			lastY = ypos;
			firstMouse = false;
		}

		float xoffset = xpos - lastX;
		float yoffset = lastY - ypos;
		// reversed since y-coordinates range from bottom to top
		lastX = xpos;
		lastY = ypos;

		//camera.ProcessMouseMovement(xoffset, yoffset);
		if (xoffset > 0) {
			camera.translate(LEFT, xoffset * 0.05);
		}
		else {
			camera.translate(RIGHT, xoffset * -0.05);
		}

		if (yoffset > 0) {
			camera.translate(DOWN, yoffset * 0.05);
		}
		else {
			camera.translate(UP, yoffset * -0.05);
		}
	}

	//Zoom 
	if (glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS &&
		glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
		if (firstMouse)
		{
			lastX = xpos;
			//lastY = ypos;
			firstMouse = false;
		}

		float xoffset = xpos - lastX;
		//float yoffset = lastY - ypos;
		// reversed since y-coordinates range from bottom to top
		lastX = xpos;
		//lastY = ypos;

		//camera.ProcessMouseMovement(xoffset, yoffset);
		if (xoffset > 0) {
			camera.translate(FORWARD, xoffset * 0.05);
		}
		else {
			camera.translate(BACKWARD, xoffset * -0.05);
		}
	}

	if ((glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_RELEASE ||
		glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_RELEASE) &&
		(glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_RELEASE ||
			glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_RELEASE) &&
		(glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_RELEASE ||
			glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_RELEASE)) {
		firstMouse = true;
	}

}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	camera.ProcessMouseScroll(yoffset);

	radius += yoffset * 0.05;
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	// make sure the viewport matches the new window dimensions; note that width and 
	// height will be significantly larger than specified on retina displays.
	glViewport(0, 0, width, height);
	sizex = width;
	sizey = height;
}

// utility function for loading a 2D texture from file
// ---------------------------------------------------
unsigned int loadTexture(char const* path, GLenum wrap)
{
	unsigned int textureID;
	glGenTextures(1, &textureID);

	int width, height, nrComponents;
	unsigned char* data = stbi_load(path, &width, &height, &nrComponents, 0);
	if (data)
	{
		GLenum format;
		if (nrComponents == 1)
			format = GL_RED;
		else if (nrComponents == 3)
			format = GL_RGB;
		else if (nrComponents == 4)
			format = GL_RGBA;

		glBindTexture(GL_TEXTURE_2D, textureID);
		glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		stbi_image_free(data);
	}
	else
	{
		std::cout << "Texture failed to load at path: " << path << std::endl;
		stbi_image_free(data);
	}

	return textureID;
}

GLuint createTransformFeedbackShader(std::vector<const GLchar*> feedbackVaryings, const char* vertexPath, const char* fragmentPath, const char* geometryPath){
	// build and compile our shader program
	// ------------------------------------
	// NOTE: the shader is created as vertex shader (and optionally geometry shader) only without a
	//       fragment shader
	std::cout << "Create Transform Feedback Shader" << std::endl;

	std::cout << "Vertex Shader";
	if (fragmentPath)
		std::cout << " | Fragment Shader";
	if (geometryPath)
		std::cout << " | Geometry Shader";
	std::cout << std::endl;

	std::string vertexCode;
	std::ifstream vShaderFile;
	std::string fragmentCode;
	std::ifstream fShaderFile;
	std::string geometryCode;
	std::ifstream gShaderFile;

	// ensure ifstream objects can throw exceptions:
	vShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);

	try
	{
		// open files
		vShaderFile.open(vertexPath);
		std::stringstream vShaderStream;
		// read file's buffer contents into streams
		vShaderStream << vShaderFile.rdbuf();
		// close file handlers
		vShaderFile.close();
		// convert stream into string
		vertexCode = vShaderStream.str();
		
		if (fragmentPath != nullptr)
		{
			fShaderFile.open(fragmentPath);
			std::stringstream fShaderStream;
			fShaderStream << fShaderFile.rdbuf();
			fShaderFile.close();
			fragmentCode = fShaderStream.str();
		}

		// if geometry shader path is present, also load a geometry shader
		if (geometryPath != nullptr)
		{
			gShaderFile.open(geometryPath);
			std::stringstream gShaderStream;
			gShaderStream << gShaderFile.rdbuf();
			gShaderFile.close();
			geometryCode = gShaderStream.str();
		}

	}
	catch (std::ifstream::failure e)
	{
		std::cout << "ERROR::SHADER::FILE_NOT_SUCCESFULLY_READ" << std::endl;
	}
	const char* vShaderCode = vertexCode.c_str();

	GLuint vertex = glCreateShader(GL_VERTEX_SHADER);
	GLuint fragment;
	GLuint geometry;

	// compile shader
	glShaderSource(vertex, 1, &vShaderCode, nullptr);
	glCompileShader(vertex);

	// if fragment shader is given, compile fragment shader
	if (fragmentPath != nullptr)
	{
		const char* fShaderCode = fragmentCode.c_str();
		fragment = glCreateShader(GL_FRAGMENT_SHADER);
		glShaderSource(fragment, 1, &fShaderCode, NULL);
		glCompileShader(fragment);
	}

	// if geometry shader is given, compile geometry shader
	if (geometryPath != nullptr)
	{
		const char* gShaderCode = geometryCode.c_str();
		geometry = glCreateShader(GL_GEOMETRY_SHADER);
		glShaderSource(geometry, 1, &gShaderCode, NULL);
		glCompileShader(geometry);
	}

	// Create program and attach shader
	GLuint program = glCreateProgram();
	glAttachShader(program, vertex);
	if (fragmentPath != nullptr)
	{
		glAttachShader(program, fragment);
	}
	if (geometryPath != nullptr)
	{
		glAttachShader(program, geometry);
	}
	
	
	// define length of output names array and array with output names
	// last parameter specifies how data should be written:
	//     GL_INTERLEAVED_ATTRIBS: Write all attributes to a single buffer object.
	//     GL_SEPARATE_ATTRIBS: Writes attributes to multiple buffer objects or at
	//                          different offsets into a buffer.
	glTransformFeedbackVaryings(
		program,
		feedbackVaryings.size(),
		(const GLchar* const*)feedbackVaryings.data(),
		GL_INTERLEAVED_ATTRIBS);

	// do linking of the program 
	// NOTE: glTransformFeedbackVaryings needs to be done before as the Linking 
	//       depends on the knowledge about the outputs
	glLinkProgram(program);

	// delete the shaders as they're linked into our program now and no longer necessary
	glDeleteShader(vertex);
	if (geometryPath != nullptr)
	{
		glDeleteShader(geometry);
	}
	if (fragmentPath != nullptr)
	{
		glDeleteShader(fragment);
	}
	
	return program;
}

void icon(GLFWwindow* window) {
	//GLFW ICON
	// a simple glfw logo
	const char* const logo[] =
	{
		"................",
		"................",
		"...0000..0......",
		"...0.....0......",
		"...0.00..0......",
		"...0..0..0......",
		"...0000..0000...",
		"................",
		"................",
		"...1111..1111...",
		"......1..1......",
		"......1..1111...",
		"...1..1.....1...",
		"...1111..1111...",
		"................",
		"................"
	};

	const unsigned char icon_colors[5][4] =
	{
		{ 0,   0,   0, 255 }, // black
		{ 255,   0,   0, 255 }, // red
		{ 0, 255,   0, 255 }, // green
		{ 0,   0, 255, 255 }, // blue
		{ 255, 255, 255, 255 }  // white
	};
	int x, y;
	unsigned char pixels[16 * 16 * 4];
	unsigned char* target = pixels;
	GLFWimage img = { 16, 16, pixels };

	for (y = 0; y < img.width; y++)
	{
		for (x = 0; x < img.height; x++)
		{
			if (logo[y][x] == '0')
				memcpy(target, icon_colors[0], 4);
			else if (logo[y][x] == '1')
				memcpy(target, icon_colors[1], 4);
			else
				memset(target, 0, 4);
			target += 4;
		}
	}

	glfwSetWindowIcon(window, 1, &img);
}

