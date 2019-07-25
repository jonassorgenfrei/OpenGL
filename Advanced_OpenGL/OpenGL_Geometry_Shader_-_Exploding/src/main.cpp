/* 
	Geometry Shader 
	-> Exploding Objects
*/

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "shader_m.h"
#include "camera.h"
#include "model.h"
#include "stb_image.h"
#include "filesystem.h"


#include "light.h"
#include "material.h"

#include <iostream>

// callbacks
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
// current Mouse position 
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow *window);
/* load texture */
unsigned int loadTexture(const char *path, GLenum wrap);

void icon(GLFWwindow* window);

// settings
const unsigned int SCR_WIDTH = 1280;
const unsigned int SCR_HEIGHT = 720;

// camera
Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
float lastX = SCR_WIDTH / 2.0;
float lastY = SCR_HEIGHT / 2.0;
bool firstMouse = true;

// timing 
float deltaTime = 0.0f; // time between current frame and last frame
float lastFrame = 0.0f; // time of last frame

int explode = 0;

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
	/* DEPTH BUFFER */
	glEnable(GL_DEPTH_TEST);


	// build and compile our shader program
	// ------------------------------------
	Shader shader(FileSystem::getPath("shader/geometry_shader.vs").c_str(), FileSystem::getPath("shader/geometry_shader.fs").c_str(), FileSystem::getPath("shader/geometry_shader.gs").c_str());
	Shader normalDisplayShader(FileSystem::getPath("shader/normalShader.vs").c_str(), FileSystem::getPath("shader/normalShader.fs").c_str(), FileSystem::getPath("shader/normalShader.gs").c_str());


	// load models
	// -----------
	Model nanosuit(FileSystem::getPath("../../content/models/nanosuit/nanosuit.obj").c_str());
	//Model nanosuit(FileSystem::getPath("../../content/models/rabbit/bunny.obj").c_str());
	float timeToExpl = glfwGetTime();
	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	// render loop
	// -----------
	while (!glfwWindowShouldClose(window))
	{
		processInput(window);
		// per-frame time logic
		// --------------------
		float currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		// render
		// ------
		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// configure transformation matrices
		glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)SCR_WIDTH / (float)SCR_HEIGHT, 1.0f, 100.0f);
		glm::mat4 view = camera.GetViewMatrix();;
		glm::mat4 model = glm::mat4(1);
		shader.use();
		shader.setMat4("projection", projection);
		shader.setMat4("view", view);
		shader.setMat4("model", model);

		shader.setInt("explodeB", explode);

		// add time component to geometry shader in the form of a uniform
		if (!explode) {
			timeToExpl = glfwGetTime();
		}
		
		shader.setFloat("time", glfwGetTime()- timeToExpl);

		// draw model
		nanosuit.Draw(shader);

		/*normalDisplayShader.use();
		normalDisplayShader.setMat4("projection", projection);
		normalDisplayShader.setMat4("view", view);
		normalDisplayShader.setMat4("model", model);
		nanosuit.Draw(shader);*/


		// glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
		// -------------------------------------------------------------------------------
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	// glfw: terminate, clearing all previously allocated GLFW resources.
	// ------------------------------------------------------------------
	glfwTerminate();
	return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window)
{	

	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);

	float cameraSpeed = 2.5f * deltaTime; // adjust accordingly

	if (glfwGetKey(window, GLFW_KEY_F2) == GLFW_PRESS)
	{
		const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
		glfwSetWindowMonitor(window, glfwGetPrimaryMonitor(), 0, 0, mode->width, mode->height, mode->refreshRate);
	}


	if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) {
		if (explode) {
			explode = 0;
		} else {
			explode = 1;
		}
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
			camera.translate(LEFT, xoffset*0.05);
		}
		else {
			camera.translate(RIGHT, xoffset*-0.05);
		}

		if (yoffset > 0) {
			camera.translate(DOWN, yoffset*0.05);
		}
		else {
			camera.translate(UP, yoffset*-0.05);
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
			camera.translate(FORWARD, xoffset*0.05);
		}
		else {
			camera.translate(BACKWARD, xoffset*-0.05);
		}
	}

	if ((	glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_RELEASE ||
			glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_RELEASE) && 
		(	glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_RELEASE ||
			glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_RELEASE) && 
		(	glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_RELEASE ||
				glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_RELEASE)) {
		firstMouse = true;
	}

	
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	camera.ProcessMouseScroll(yoffset);
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	// make sure the viewport matches the new window dimensions; note that width and 
	// height will be significantly larger than specified on retina displays.
	glViewport(0, 0, width, height);
}

// utility function for loading a 2D texture from file
// ---------------------------------------------------
unsigned int loadTexture(char const *path, GLenum wrap)
{
	unsigned int textureID;
	glGenTextures(1, &textureID);

	int width, height, nrComponents;
	unsigned char *data = stbi_load(path, &width, &height, &nrComponents, 0);
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