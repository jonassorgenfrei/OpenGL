/* 
 * Order Independent Transparency
 * Depth Peeling
 */

#include <glad/glad.h>
#include <GLFW/glfw3.h>

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
void processInput(GLFWwindow *window);

glm::mat4 calculate_model_matrix(const glm::vec3& position, const glm::vec3& rotation = glm::vec3(0.0f), const glm::vec3& scale = glm::vec3(1.0f));

void icon(GLFWwindow* window);


// settings
const unsigned int SCR_WIDTH = 1280;
const unsigned int SCR_HEIGHT = 720;

int Fpressed = 0;
int Uppressed = 0;
int Downpressed = 0;

int numPasses = 4;             // number of transparent layers
// camera
Camera camera(glm::vec3(0.0f, 0.0f, 5.0f));
float lastX = SCR_WIDTH / 2.0;
float lastY = SCR_HEIGHT / 2.0;
bool firstMouse = true;
bool printFPS = false;

// timing 
float deltaTime = 0.0f; // time between current frame and last frame
float lastFrame = 0.0f; // time of last frame

int main()
{
	// glfw: initialize and configure
	// ------------------------------
	glfwInit();
	// needs opengl 4.0 to use blending to multiple render targets
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
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

	// build and compile shader program(s)
	// ------------------------------------
	Shader solidShader(FileSystem::getSamplePath("shader/solid.vert").c_str(), FileSystem::getSamplePath("shader/solid.frag").c_str());
	Shader transparentShader(FileSystem::getSamplePath("shader/transparent.vert").c_str(), FileSystem::getSamplePath("shader/transparent.frag").c_str());
	Shader compositeShader(FileSystem::getSamplePath("shader/composite.vert").c_str(), FileSystem::getSamplePath("shader/composite.frag").c_str());
	Shader screenShader(FileSystem::getSamplePath("shader/screen.vert").c_str(), FileSystem::getSamplePath("shader/screen.frag").c_str());

	// set up vertex data (and buffer(s)) and configure vertex attributes
	// ------------------------------------------------------------------

	// define quad structure
	float quadVertices[] = {
		// positions		// uv
		-1.0f, -1.0f, 0.0f,	0.0f, 0.0f,
		 1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
		 1.0f,  1.0f, 0.0f, 1.0f, 1.0f,

		 1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
		-1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
		-1.0f, -1.0f, 0.0f, 0.0f, 0.0f
	};
	
	// quad VAO
	unsigned int quadVAO, quadVBO;
	glGenVertexArrays(1, &quadVAO);
	glGenBuffers(1, &quadVBO);
	glBindVertexArray(quadVAO);
	glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);	// define positions
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));	// define uvs
	glBindVertexArray(0);	// unbined vertex array

	// set up framebuffers and their texture attachments
	// ------------------------------------------------------------------
	unsigned int opaqueFBO;
	unsigned int depthTextureOpaque;
	unsigned int opaqueColorTex;

	// Erstelle FBO
	glGenFramebuffers(1, &opaqueFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, opaqueFBO);

	// Farb-Textur
	glGenTextures(1, &opaqueColorTex);
	glBindTexture(GL_TEXTURE_2D, opaqueColorTex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_HALF_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, opaqueColorTex, 0);

	// Depth-Textur
	glGenTextures(1, &depthTextureOpaque);
	glBindTexture(GL_TEXTURE_2D, depthTextureOpaque);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SCR_WIDTH, SCR_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthTextureOpaque, 0);

	// Check FBO
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		std::cout << "ERROR::FRAMEBUFFER:: Opaque framebuffer is not complete!" << std::endl;

	glBindFramebuffer(GL_FRAMEBUFFER, 0);


	unsigned int peelFBO[2];
	unsigned int colorTex[2];
	unsigned int depthTex[2]; // Depth Peeling Buffers

	glGenFramebuffers(2, peelFBO);
	glGenTextures(2, colorTex);
	glGenTextures(2, depthTex);

	for (int i = 0; i < 2; i++) {
		glBindTexture(GL_TEXTURE_2D, colorTex[i]);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_HALF_FLOAT, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		glBindTexture(GL_TEXTURE_2D, depthTex[i]);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SCR_WIDTH, SCR_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		glBindFramebuffer(GL_FRAMEBUFFER, peelFBO[i]);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTex[i], 0);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthTex[i], 0);
	}

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		std::cout << "ERROR::FRAMEBUFFER:: Transparent framebuffer is not complete!" << std::endl;

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	unsigned int compositeFBO;
	unsigned int compositeTexture;

	glGenFramebuffers(1, &compositeFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, compositeFBO);

	// color-textur
	glGenTextures(1, &compositeTexture);
	glBindTexture(GL_TEXTURE_2D, compositeTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_HALF_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, compositeTexture, 0);

	// depth-textur optional
	unsigned int compositeDepth;
	glGenTextures(1, &compositeDepth);
	glBindTexture(GL_TEXTURE_2D, compositeDepth);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SCR_WIDTH, SCR_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, compositeDepth, 0);

	// Check FBO
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		std::cout << "ERROR::FRAMEBUFFER:: Composite framebuffer is not complete!" << std::endl;

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	compositeShader.use();
	compositeShader.setInt("peelTex", 0);
	compositeShader.setInt2("screenSize", SCR_WIDTH, SCR_HEIGHT);

	transparentShader.use();
	transparentShader.setInt("prevDepthTex", 0);
	transparentShader.setInt("opaqueDepthTex", 1);


	// set up transformation matrices
	// ------------------------------------------------------------------
	// set z axis arbitraily as the rendering ins order-independent
	glm::mat4 redModelMat = calculate_model_matrix(glm::vec3(0.0f, 0.0f, -1.0f));
	glm::mat4 greenModelMat = calculate_model_matrix(glm::vec3(0.0f, 0.0f, 0.0f));
	glm::mat4 blueModelMat = calculate_model_matrix(glm::vec3(0.0f, 0.0f, 2.0f));
	glm::mat4 orangeModelMat = calculate_model_matrix(glm::vec3(0.0f, 0.0f, 1.5f));

	// set up intermediate variables
	// ------------------------------------------------------------------
	glm::vec4 zeroFillerVec(0.0f);
	glm::vec4 oneFillerVec(1.0f);

	int frameCount = 0;
	double previousTime = glfwGetTime();
	// render loop
	// -----------
	while (!glfwWindowShouldClose(window))
	{
		// glEnable(GL_CULL_FACE);
		// glCullFace(GL_BACK);
	
		// Set frame time
		GLfloat currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame; // time per frame 
		lastFrame = currentFrame;

		// fps
		if (printFPS) {
			frameCount++;
			// If a second has passed.
			if (currentFrame - previousTime >= 1.0)
			{
				std::cout << "FPS:" << frameCount << std::endl;
				frameCount = 0;
				previousTime = currentFrame;
			}
		} else {
			frameCount = 0;
			previousTime = glfwGetTime();
		}

		// camera matrices
		glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
		glm::mat4 view = camera.GetViewMatrix();
		glm::mat4 vp = projection * view;


		// Check and call events
		processInput(window);

		// render
		// ------

		// draw solid objects (solid pass)
		// ------
		glBindFramebuffer(GL_FRAMEBUFFER, opaqueFBO);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LESS);
		glDepthMask(GL_TRUE);
		glDisable(GL_BLEND);

		// use solid shader
		solidShader.use();

		// draw red quad
		solidShader.setMat4("mvp", vp * redModelMat);
		solidShader.setVec3("color", glm::vec3(1.0f, 0.0f, 0.0f));
		glBindVertexArray(quadVAO);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		
		// --- Depth Peeling Setup ---
		// Depth peeling per-frame pipeline:
		// 1) clear composite buffer, 2) peel N transparent layers against opaque+prev depth, 3) blend layers, 4) present
		int curr = 0, prev = 1;        // ping-pong buffer IDs

		// Clear composite target for transparent accumulation
		glBindFramebuffer(GL_FRAMEBUFFER, compositeFBO);
		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glDepthMask(GL_TRUE);
		glEnable(GL_DEPTH_TEST);

		// Depth peeling loop
			// Peel front-to-back: each pass compares to opaque depth on pass 0, then previous peel depth afterwards
		for (int pass = 0; pass < numPasses; pass++) {
			std::swap(curr, prev);

			glBindFramebuffer(GL_FRAMEBUFFER, peelFBO[curr]);
			glm::vec4 zeroVec(0.0f);
			glm::vec4 oneVec(1.0f);
			glClearBufferfv(GL_COLOR, 0, &zeroVec[0]);
			glClearBufferfv(GL_DEPTH, 0, &oneVec[0]);

			// --- Transparent pass ---
			transparentShader.use();
			glActiveTexture(GL_TEXTURE0); // prev peel depth
			// bind opaque depth once on texture unit 1
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, depthTextureOpaque);
			glActiveTexture(GL_TEXTURE0);

			if (pass == 0) {
				glBindTexture(GL_TEXTURE_2D, depthTextureOpaque); // compare against opaque depth
			}
			else {
				glBindTexture(GL_TEXTURE_2D, depthTex[prev]);     // compare against previous peel
			}

			transparentShader.setInt("prevDepthTex", 0);
			transparentShader.setInt("pass", pass);

			glDepthFunc(GL_LESS);   // keep fragments closer than prevDepth
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

			// Draw transparent objects
			glBindVertexArray(quadVAO);
			transparentShader.setMat4("mvp", vp * greenModelMat);
			transparentShader.setVec4("color", glm::vec4(0.0f, 1.0f, 0.0f, 0.5f));
			glDrawArrays(GL_TRIANGLES, 0, 6);

			transparentShader.setMat4("mvp", vp * blueModelMat);
			transparentShader.setVec4("color", glm::vec4(0.0f, 0.0f, 1.0f, 0.5f));
			glDrawArrays(GL_TRIANGLES, 0, 6);

			transparentShader.setMat4("mvp", vp * orangeModelMat);
			transparentShader.setVec4("color", glm::vec4(1.0f, 0.5f, 0.0f, 0.5f));
			glDrawArrays(GL_TRIANGLES, 0, 6);
			

			// --- Composite this layer ---
			// Blend current peeled layer into accumulation buffer (premultiplied alpha)
			glDisable(GL_DEPTH_TEST);
			glDepthMask(GL_FALSE);
			glBindFramebuffer(GL_FRAMEBUFFER, compositeFBO);
			glEnable(GL_BLEND);
			glBlendFuncSeparate(GL_ONE_MINUS_DST_ALPHA, GL_ONE, GL_ONE_MINUS_DST_ALPHA, GL_ONE);

			compositeShader.use();
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, colorTex[curr]); // current peel

			glBindVertexArray(quadVAO);
			glDrawArrays(GL_TRIANGLES, 0, 6);
			glDepthMask(GL_TRUE);
			glEnable(GL_DEPTH_TEST);
		}

		// --- Final Pass: Render to screen ---
		// Present: draw opaque buffer, then overlay accumulated transparents
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glDisable(GL_DEPTH_TEST);
		glClear(GL_COLOR_BUFFER_BIT);

		screenShader.use();
		glDisable(GL_BLEND);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, opaqueColorTex);
		screenShader.setInt("screenTex", 0);

		glBindVertexArray(quadVAO);
		glDrawArrays(GL_TRIANGLES, 0, 6);

		// Overlay accumulated transparent layers
		glEnable(GL_BLEND);
		glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
		glBindTexture(GL_TEXTURE_2D, compositeTexture);
		glDrawArrays(GL_TRIANGLES, 0, 6);

		// Reset states
		glDisable(GL_BLEND);
		glDepthMask(GL_TRUE);

		// glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
		// -------------------------------------------------------------------------------
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	// optional: de-allocate all resources once they've outlived their purpose:
	// ------------------------------------------------------------------------
	glDeleteVertexArrays(1, &quadVAO);
	glDeleteBuffers(1, &quadVBO);

	
	// glfw: terminate, clearing all previously allocated GLFW resources.
	// ------------------------------------------------------------------
	glfwTerminate();
	return EXIT_SUCCESS;
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


	if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS) {
		Fpressed = true;
	}

	if (Fpressed && glfwGetKey(window, GLFW_KEY_F) == GLFW_RELEASE) {
		Fpressed = false;
		if (!printFPS) {
			std::cout << "PRINT FPS" << std::endl;
		}

		printFPS = !printFPS;
	}

	if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
		Uppressed = true;
	}

	if (Uppressed && glfwGetKey(window, GLFW_KEY_UP) == GLFW_RELEASE) {
		Uppressed = false;
		numPasses += 1;
		std::cout << "Peel-Passes" << numPasses << std::endl;
	
	}

	if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
		Downpressed = true;
	}

	if (Downpressed && glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_RELEASE) {
		Downpressed = false;
		numPasses = max(0, numPasses-1);
		std::cout << "Peel-Passes" << numPasses << std::endl;		
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

// generate a model matrix
// ---------------------------------------------------------------------------------------------------------
glm::mat4 calculate_model_matrix(const glm::vec3& position, const glm::vec3& rotation, const glm::vec3& scale)
{
	glm::mat4 trans = glm::mat4(1.0f);

	trans = glm::translate(trans, position);
	trans = glm::rotate(trans, glm::radians(rotation.x), glm::vec3(1.0, 0.0, 0.0));
	trans = glm::rotate(trans, glm::radians(rotation.y), glm::vec3(0.0, 1.0, 0.0));
	trans = glm::rotate(trans, glm::radians(rotation.z), glm::vec3(0.0, 0.0, 1.0));
	trans = glm::scale(trans, scale);

	return trans;
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
