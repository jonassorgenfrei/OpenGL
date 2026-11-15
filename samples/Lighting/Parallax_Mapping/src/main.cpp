/* 
 *	Parallax Mapping
 *		Belongs to the family of displacement mapping
 *		Displace or offset vertices based on geometrical information (stored inside a texture)
 *		Parallax Mapping -> in Tangent Space
 */

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "stb_image.h"

#include "modules/shader_m.h"
#include "modules/camera.h"
#include "modules/filesystem.h"
#include "modules/window.h"
#include "modules/help_overlay.h"

#include <iostream>

// callbacks
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
// current Mouse position 
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow *window);
void drawPlane(void);
/* load texture */
unsigned int loadTexture(const char *path);

// settings
const unsigned int SCR_WIDTH = 1280;
const unsigned int SCR_HEIGHT = 720;
float heightScale = 0.1;
bool parallaxEnabled = true;
bool normalMappingEnabled = true;
bool showNormals = false;
bool parallaxKeyPressed = false;
bool normalMappingKeyPressed = false;
bool showNormalsKeyPressed = false;
bool textureKeyPressed = false;
int parallaxMethod = 3;
int texture = 0; // 0 - wood, 1 - bricks

// camera
Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
float lastX = SCR_WIDTH / 2.0;
float lastY = SCR_HEIGHT / 2.0;
bool firstMouse = true;

// timing 
float deltaTime = 0.0f; // time between current frame and last frame
float lastFrame = 0.0f; // time of last frame
unsigned int quadVAO = 0;
unsigned int quadVBO = 0;

int main()
{
	// glfw: initialize and configure
	// ------------------------------
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

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
		sample_help::shutdown();
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
	sample_help::initialize(window, {
		{"Esc", "Exit sample"},
		{"F2", "Switch to fullscreen"},
		{"Alt + LMB", "Orbit camera"},
		{"Alt + MMB", "Pan camera"},
		{"Alt + RMB", "Dolly camera"},
		{"Mouse wheel", "Zoom camera"},
		{"P", "Toggle parallax mapping"},
		{"N", "Toggle normal map / geometric normal"},
		{"V", "Visualize the selected normal"},
		{"1 / 2 / 3", "Basic / steep / occlusion parallax"},
		{"T", "Switch wood / brick textures"},
		{"Q / E", "Decrease / increase height scale"}
	});

	// configure global OpenGL state
	// -----------------------------
	/* DEPTH BUFFER */
	glEnable(GL_DEPTH_TEST);


	// build and compile shader program(s)
	// ------------------------------------
	Shader shader(FileSystem::getSamplePath("shader/normalShader.vert").c_str(), FileSystem::getSamplePath("shader/normalShader.frag").c_str());
	
	// load textures
	// -------------
	/* Bricks */
	unsigned int bricks_diffuseMap = loadTexture(FileSystem::getPath("content/images/bricks2.jpg").c_str());
	unsigned int bricks_depthMap = loadTexture(FileSystem::getPath("content/images/bricks2_disp.jpg").c_str());
	unsigned int bricks_normalMap = loadTexture(FileSystem::getPath("content/images/bricks2_normal.jpg").c_str());
	

	/* Wood */
	unsigned int wood_diffuseMap = loadTexture(FileSystem::getPath("content/images/wood.png").c_str());
	unsigned int wood_depthMap = loadTexture(FileSystem::getPath("content/images/toy_box_disp.png").c_str());
	unsigned int wood_normalMap = loadTexture(FileSystem::getPath("content/images/toy_box_normal.png").c_str());

	// Shader configuration
	// --------------------
	shader.use();
	shader.setInt("diffuseMap", 0);
	shader.setInt("normalMap", 1);
	shader.setInt("depthMap", 2);

	// lighting info
	// -------------
	glm::vec3 lightPos(0.5f, 1.0f, 0.3f);

	// render loop
	// -----------
	while (!glfwWindowShouldClose(window))
	{
		glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK);

		// Set frame time
		GLfloat currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		// Check and call events
		processInput(window);

		// render
		// ------
		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		shader.use();
		glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (GLfloat)SCR_WIDTH / (GLfloat)SCR_HEIGHT, 0.1f, 100.0f);
		shader.setMat4("projection", projection);
		shader.setMat4("view", camera.GetViewMatrix());

		glm::mat4 model(1.0f);
		model = glm::rotate(model, glm::radians((float)glfwGetTime() * -10.0f), glm::normalize(glm::vec3(1.0, 0.0, 1.0))); // rotate the quad to show parallax mapping from multiple directions
		//model = glm::rotate(model, glm::radians((float)90), glm::vec3(-1.0, 0.0, 0.0));
		shader.setMat4("model", model);
		// set Eye 
		shader.setVec3("viewPos", camera.Position);
		shader.setVec3("lightPos", lightPos);
		shader.setFloat("height_scale", heightScale);
		shader.setBool("parallaxEnabled", parallaxEnabled);
		shader.setBool("normalMappingEnabled", normalMappingEnabled);
		shader.setBool("showNormals", showNormals);
		shader.setInt("parallaxMethod", parallaxMethod);

		if (texture == 1) {
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, bricks_diffuseMap);
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, bricks_normalMap);
			glActiveTexture(GL_TEXTURE2);
			glBindTexture(GL_TEXTURE_2D, bricks_depthMap);
		}
		else {
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, wood_diffuseMap);
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, wood_normalMap);
			glActiveTexture(GL_TEXTURE2);
			glBindTexture(GL_TEXTURE_2D, wood_depthMap);
		}
		

		drawPlane();

		// glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
		// -------------------------------------------------------------------------------
		sample_help::render();
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	// optional: de-allocate all resources once they've outlived their purpose:
	// ------------------------------------------------------------------------
	glDeleteVertexArrays(1, &quadVAO);
	glDeleteBuffers(1, &quadVBO);
	unsigned int textures[] = {
		bricks_diffuseMap, bricks_depthMap, bricks_normalMap,
		wood_diffuseMap, wood_depthMap, wood_normalMap
	};
	glDeleteTextures(6, textures);
	
	// glfw: terminate, clearing all previously allocated GLFW resources.
	// ------------------------------------------------------------------
	sample_help::shutdown();
	glfwTerminate();
	return 0;
}

// renders a 1x1 quad in NDC with manually calculated tangent vectors
// ------------------------------------------------------------------
void drawPlane() {
	if (quadVAO == 0) {
		// positions
		glm::vec3 pos1(-1.0, 1.0, 0.0);
		glm::vec3 pos2(-1.0, -1.0, 0.0);
		glm::vec3 pos3(1.0, -1.0, 0.0);
		glm::vec3 pos4(1.0, 1.0, 0.0);

		// texture coordinates
		glm::vec2 uv1(0.0, 1.0);
		glm::vec2 uv2(0.0, 0.0);
		glm::vec2 uv3(1.0, 0.0);
		glm::vec2 uv4(1.0, 1.0);

		// normal vector
		glm::vec3 nm(0.0, 0.0, 1.0);

		// calculate tangent/bitangent vectors of both triangles 
		glm::vec3 tangent1, bitangent1;
		glm::vec3 tangent2, bitangent2;

		// triangle 1
		// ----------
		// calculating first triangle's edges and delta UV coordinates.
		glm::vec3 edge1 = pos2 - pos1;
		glm::vec3 edge2 = pos3 - pos1;
		glm::vec2 deltaUV1 = uv2 - uv1;
		glm::vec2 deltaUV2 = uv3 - uv1;

		// calculating tangents and bitangents
		//pre-calculate fractional part of the equation
		float f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

		tangent1.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
		tangent1.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
		tangent1.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);
		tangent1 = glm::normalize(tangent1); // make sure it ends up as unit vector

		bitangent1.x = f * (-deltaUV2.x * edge1.x + deltaUV1.x * edge2.x);
		bitangent1.y = f * (-deltaUV2.x * edge1.y + deltaUV1.x * edge2.y);
		bitangent1.z = f * (-deltaUV2.x * edge1.z + deltaUV1.x * edge2.z);
		bitangent1 = glm::normalize(bitangent1); // make sure it ends up as unit vector

		// triangle 2
		// ----------
		// Calculate the basis for the second triangle.
		edge1 = pos3 - pos1;
		edge2 = pos4 - pos1;
		deltaUV1 = uv3 - uv1;
		deltaUV2 = uv4 - uv1;

		// calculating tangents and bitangents
		//pre-calculate fractional part of the equation
		f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

		tangent2.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
		tangent2.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
		tangent2.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);
		tangent2 = glm::normalize(tangent2); // make sure it ends up as unit vector

		bitangent2.x = f * (-deltaUV2.x * edge1.x + deltaUV1.x * edge2.x);
		bitangent2.y = f * (-deltaUV2.x * edge1.y + deltaUV1.x * edge2.y);
		bitangent2.z = f * (-deltaUV2.x * edge1.z + deltaUV1.x * edge2.z);
		bitangent2 = glm::normalize(bitangent2); // make sure it ends up as unit vector

		float quadVertices[] = {
			// positions            // normal         // texcoords  // tangent                          // bitangent
			pos1.x, pos1.y, pos1.z, nm.x, nm.y, nm.z, uv1.x, uv1.y, tangent1.x, tangent1.y, tangent1.z, bitangent1.x, bitangent1.y, bitangent1.z,
			pos2.x, pos2.y, pos2.z, nm.x, nm.y, nm.z, uv2.x, uv2.y, tangent1.x, tangent1.y, tangent1.z, bitangent1.x, bitangent1.y, bitangent1.z,
			pos3.x, pos3.y, pos3.z, nm.x, nm.y, nm.z, uv3.x, uv3.y, tangent1.x, tangent1.y, tangent1.z, bitangent1.x, bitangent1.y, bitangent1.z,

			pos1.x, pos1.y, pos1.z, nm.x, nm.y, nm.z, uv1.x, uv1.y, tangent2.x, tangent2.y, tangent2.z, bitangent2.x, bitangent2.y, bitangent2.z,
			pos3.x, pos3.y, pos3.z, nm.x, nm.y, nm.z, uv3.x, uv3.y, tangent2.x, tangent2.y, tangent2.z, bitangent2.x, bitangent2.y, bitangent2.z,
			pos4.x, pos4.y, pos4.z, nm.x, nm.y, nm.z, uv4.x, uv4.y, tangent2.x, tangent2.y, tangent2.z, bitangent2.x, bitangent2.y, bitangent2.z
		};

		// configure plane VAO
		glGenVertexArrays(1, &quadVAO);
		glGenBuffers(1, &quadVBO);
		glBindVertexArray(quadVAO);
		glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE, 14*sizeof(float), (void*)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)(3*sizeof(float)));
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)(6 * sizeof(float)));
		glEnableVertexAttribArray(3);
		glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)(8 * sizeof(float)));
		glEnableVertexAttribArray(4);
		glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)(11 * sizeof(float)));
	}
	glBindVertexArray(quadVAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindVertexArray(0);

}


// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window)
{	
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);

	if (glfwGetKey(window, GLFW_KEY_F2) == GLFW_PRESS)
	{
		const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
		glfwSetWindowMonitor(window, glfwGetPrimaryMonitor(), 0, 0, mode->width, mode->height, mode->refreshRate);
	}

	if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS && !parallaxKeyPressed) {
		parallaxEnabled = !parallaxEnabled;
		parallaxKeyPressed = true;
	}
	if (glfwGetKey(window, GLFW_KEY_P) == GLFW_RELEASE)
		parallaxKeyPressed = false;

	if (glfwGetKey(window, GLFW_KEY_N) == GLFW_PRESS && !normalMappingKeyPressed) {
		normalMappingEnabled = !normalMappingEnabled;
		normalMappingKeyPressed = true;
	}
	if (glfwGetKey(window, GLFW_KEY_N) == GLFW_RELEASE)
		normalMappingKeyPressed = false;

	if (glfwGetKey(window, GLFW_KEY_V) == GLFW_PRESS && !showNormalsKeyPressed) {
		showNormals = !showNormals;
		showNormalsKeyPressed = true;
	}
	if (glfwGetKey(window, GLFW_KEY_V) == GLFW_RELEASE)
		showNormalsKeyPressed = false;

	if (glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS && !textureKeyPressed) {
		texture = 1 - texture;
		textureKeyPressed = true;
	}
	if (glfwGetKey(window, GLFW_KEY_T) == GLFW_RELEASE)
		textureKeyPressed = false;

	if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS)
		parallaxMethod = 1;
	else if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS)
		parallaxMethod = 2;
	else if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS)
		parallaxMethod = 3;

	const float heightChange = 0.1f * deltaTime;
	if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
		heightScale = glm::max(0.0f, heightScale - heightChange);
	else if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
		heightScale = glm::min(0.2f, heightScale + heightChange);
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
unsigned int loadTexture(char const *path)
{
	unsigned int textureID;
	glGenTextures(1, &textureID);

	int width, height, nrComponents;
	unsigned char *data = stbi_load(path, &width, &height, &nrComponents, 0);
	if (data)
	{
		GLenum internalFormat;
		GLenum dataFormat;
		if (nrComponents == 1) {
			internalFormat = dataFormat = GL_RED;
		}
		else if (nrComponents == 2) {
			internalFormat = dataFormat = GL_RG;
		}
		else if (nrComponents == 3) {
			internalFormat = dataFormat = GL_RGB;
		}
		else if (nrComponents == 4) {
			internalFormat = dataFormat = GL_RGBA;
		}

		glBindTexture(GL_TEXTURE_2D, textureID);
		glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, dataFormat, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, internalFormat == GL_RGBA ? GL_CLAMP_TO_EDGE : GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, internalFormat == GL_RGBA ? GL_CLAMP_TO_EDGE : GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		stbi_image_free(data);
	}
	else
	{
		std::cout << "Texture failed to load at path: " << path << std::endl;
		stbi_image_free(data);
		glDeleteTextures(1, &textureID);
		return 0;
	}

	return textureID;
}
