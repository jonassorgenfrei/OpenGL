/** 
 *	Compute Shader
 */

#include <iostream>

// OpenGL (Glad & GLFW)
#include <glad/glad.h>
#include <GLFW/glfw3.h>

// GLM 
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
#include "modules/window.h"

#include "gBuffer.h"

#include "water.h"
#include "ground.h"

/** Makro zum Definieren der Uniform-Locations zur Kommunikation mit dem Shaderprogramm (Location ist konstant) */
#define DEFINE_UNIFORM_LOCATION(PROGRAM, NAME, LOCATION) static const GLuint PROGRAM##_##NAME = LOCATION
/** Makro zum Definieren der Uniform-Locations zur Kommunikation mit dem Shaderprogramm (Location ist änderbar) */
#define DEFINE_UNIFORM_LOCATION_MUTABLE(PROGRAM, NAME) static GLuint PROGRAM##_##NAME = -1

// Shader Locations
// ----------------

// GBuffer Program
DEFINE_UNIFORM_LOCATION(gbufferProgram, ModelMatrix, 0);
DEFINE_UNIFORM_LOCATION(gbufferProgram, ViewMatrix, 1);
DEFINE_UNIFORM_LOCATION(gbufferProgram, ProjectionMatrix, 2);
DEFINE_UNIFORM_LOCATION(gbufferProgram, NormalMatrix, 3);
DEFINE_UNIFORM_LOCATION_MUTABLE(gbufferProgram, ObjectMaterialTexture);
DEFINE_UNIFORM_LOCATION_MUTABLE(gbufferProgram, ObjectMaterial_color);
DEFINE_UNIFORM_LOCATION_MUTABLE(gbufferProgram, ObjectMaterial_ambientReflection);
DEFINE_UNIFORM_LOCATION_MUTABLE(gbufferProgram, ObjectMaterial_diffuseReflection);
DEFINE_UNIFORM_LOCATION_MUTABLE(gbufferProgram, ObjectMaterial_specularReflection);
DEFINE_UNIFORM_LOCATION_MUTABLE(gbufferProgram, ObjectMaterial_shininess);
DEFINE_UNIFORM_LOCATION_MUTABLE(gbufferProgram, ObjectMaterial_hasTexture);

// Light Program
DEFINE_UNIFORM_LOCATION(lightProgram, ViewerInverseViewProjectionMatrix, 0);
DEFINE_UNIFORM_LOCATION(lightProgram, ViewerViewMatrix, 1);
DEFINE_UNIFORM_LOCATION(lightProgram, ShadowMapViewMatrix, 2);
DEFINE_UNIFORM_LOCATION(lightProgram, ShadowMapProjectionMatrix, 3);
DEFINE_UNIFORM_LOCATION_MUTABLE(lightProgram, Sun_Direction);
DEFINE_UNIFORM_LOCATION_MUTABLE(lightProgram, Sun_AmbientColor);
DEFINE_UNIFORM_LOCATION_MUTABLE(lightProgram, Sun_DiffuseColor);
DEFINE_UNIFORM_LOCATION_MUTABLE(lightProgram, Sun_SpecularColor);
DEFINE_UNIFORM_LOCATION_MUTABLE(lightProgram, Sun_Shininess);
// Texures
DEFINE_UNIFORM_LOCATION(lightProgram, ColorBuffer, 0);
DEFINE_UNIFORM_LOCATION(lightProgram, ReflectionBuffer, 1);
DEFINE_UNIFORM_LOCATION(lightProgram, NormalBuffer, 2);
DEFINE_UNIFORM_LOCATION(lightProgram, DepthBuffer, 3);
DEFINE_UNIFORM_LOCATION(lightProgram, ShadowMap, 4);

// Water Program
DEFINE_UNIFORM_LOCATION(waterProgram, ModelMatrix, 0);
DEFINE_UNIFORM_LOCATION(waterProgram, ViewMatrix, 1);
DEFINE_UNIFORM_LOCATION(waterProgram, ProjectionMatrix, 2);
DEFINE_UNIFORM_LOCATION(waterProgram, NormalMatrix, 3);
DEFINE_UNIFORM_LOCATION(waterProgram, ViewerInverseViewProjectionMatrix, 4);
DEFINE_UNIFORM_LOCATION(waterProgram, ViewerViewMatrix, 5);
DEFINE_UNIFORM_LOCATION_MUTABLE(waterProgram, Sun_Direction);
DEFINE_UNIFORM_LOCATION_MUTABLE(waterProgram, Sun_AmbientColor);
DEFINE_UNIFORM_LOCATION_MUTABLE(waterProgram, Sun_DiffuseColor);
DEFINE_UNIFORM_LOCATION_MUTABLE(waterProgram, Sun_SpecularColor);
DEFINE_UNIFORM_LOCATION_MUTABLE(waterProgram, Sun_Shininess);
// Textures
DEFINE_UNIFORM_LOCATION(waterProgram, ColorBuffer, 0);
DEFINE_UNIFORM_LOCATION(waterProgram, ReflectionBuffer, 1);
DEFINE_UNIFORM_LOCATION(waterProgram, NormalBuffer, 2);
DEFINE_UNIFORM_LOCATION(waterProgram, DepthBuffer, 3);
DEFINE_UNIFORM_LOCATION(waterProgram, EnvironmentMap, 4);

// Sky
DEFINE_UNIFORM_LOCATION(skyProgram, ModelMatrix, 0);
DEFINE_UNIFORM_LOCATION(skyProgram, ViewMatrix, 1);
DEFINE_UNIFORM_LOCATION(skyProgram, ProjectionMatrix, 2);
DEFINE_UNIFORM_LOCATION(skyProgram, EnvironmentMap, 0);

// Function Interfaces
// -------------------

// callbacks
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
// current Mouse position 
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow *window);
/* load texture */
unsigned int loadTexture(const char *path, GLenum wrap);

// Render Geometry
void renderQuad();
void renderCube();

// Global Variables
// ----------------

// Screen settings
const unsigned int SCR_WIDTH = 1920;
const unsigned int SCR_HEIGHT = 1080;

// input 
float lastX = SCR_WIDTH / 2.0;
float lastY = SCR_HEIGHT / 2.0;
bool firstMouse = true;

bool wireframeKeyPressed = false;
bool wireframe = false;
bool renderBuffersKeyPressed = false;
bool renderBuffers = false;
bool runningKeyPressed = false;
bool running = true;

// camera
Camera camera(glm::vec3(-0.35f, 0.3f, 2.5f));
// timing 
double deltaTime = 0.0; // time between current frame and last frame
double lastFrame = 0.0; // time of last frame
double simulationTime = 0.0;

// main-Program
int main()
{
	// glfw: initialize and configure
	// ------------------------------
	if (!glfwInit()) {
		std::cout << " Panic: " << std::endl;
		std::cout << "Could not initialize GLFW!" << std::endl;
		return -1;
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);	// Compute Shader from Version 4.3 and up
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

	// Capture User Input 
	// ------------------
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetScrollCallback(window, scroll_callback);
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);	//tell GLFW to capture our mouse

	// glad: load all OpenGL function pointers
	// ---------------------------------------
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
		return -1;
	}

	std::cout << "OpenGL-Version: " << glGetString(GL_VERSION) << std::endl;

	// Window Logo 
	icon(window);

	// Shader Programs
	// build and compile our shader program & save Uniform Shader Locations
	// ------------------------------------
	// GBuffer
	Shader gBufferProgram(FileSystem::getSamplePath("shader/gBuffer/gBuffer.vert").c_str(), FileSystem::getSamplePath("shader/gBuffer/gBuffer.frag").c_str());
	
	gbufferProgram_ObjectMaterial_color = gBufferProgram.getLocation("ObjectMaterial.color");
	gbufferProgram_ObjectMaterial_ambientReflection = gBufferProgram.getLocation("ObjectMaterial.ambientReflection");
	gbufferProgram_ObjectMaterial_diffuseReflection = gBufferProgram.getLocation("ObjectMaterial.diffuseReflection");
	gbufferProgram_ObjectMaterial_specularReflection = gBufferProgram.getLocation("ObjectMaterial.specularReflection");
	gbufferProgram_ObjectMaterial_shininess = gBufferProgram.getLocation("ObjectMaterial.shininess");
	gbufferProgram_ObjectMaterial_hasTexture = gBufferProgram.getLocation("ObjectMaterial.hasTexture");
	
	// Light
	Shader lightProgram(FileSystem::getSamplePath("shader/light/light.vert").c_str(), FileSystem::getSamplePath("shader/light/light.frag").c_str());
	lightProgram_Sun_Direction = lightProgram.getLocation("Sun.direction");
	lightProgram_Sun_AmbientColor = lightProgram.getLocation("Sun.ambientColor");
	lightProgram_Sun_DiffuseColor = lightProgram.getLocation("Sun.diffuseColor");
	lightProgram_Sun_SpecularColor = lightProgram.getLocation("Sun.specularColor");
	lightProgram_Sun_Shininess = lightProgram.getLocation("Sun.shininess");

	// Water Program
	Shader waterProgram(FileSystem::getSamplePath("shader/water/water.vert").c_str(), FileSystem::getSamplePath("shader/water/water.frag").c_str());
	waterProgram_Sun_Direction = waterProgram.getLocation("Sun.direction");
	waterProgram_Sun_AmbientColor = waterProgram.getLocation("Sun.ambientColor");
	waterProgram_Sun_DiffuseColor = waterProgram.getLocation("Sun.diffuseColor");
	waterProgram_Sun_SpecularColor = waterProgram.getLocation("Sun.specularColor");
	waterProgram_Sun_Shininess = waterProgram.getLocation("Sun.shininess");

	// Sky
	Shader skyProgram(FileSystem::getSamplePath("shader/sky/sky.vert").c_str(), FileSystem::getSamplePath("shader/sky/sky.frag").c_str());
	
	// G-Buffer
	// --------
	GBuffer gBuffer = GBuffer();
	gBuffer.init(SCR_WIDTH, SCR_HEIGHT);

	// Water
	// ----------
	Water water = Water();
	
	// Ground
	// ------
	Ground ground = Ground();

	// Textures
	// -------

	// Environment Map
	GLuint environmentMap = loadTexture(FileSystem::getPath("content/images/environment.jpg").c_str(), GL_REPEAT);
	GLuint sandTexture = loadTexture(FileSystem::getPath("content/images/sand.png").c_str(), GL_REPEAT);

	// configure global opengl state
	// -----------------------------
	glfwSwapInterval(1);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	
	// DEPTH BUFFER
	glClearDepth(1);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
		
	// Face Culling
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glFrontFace(GL_CCW);

	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	// Light 
	// -----
	glm::vec3 sunPosition(-1.0,3.0,1.0);
	glm::vec3 sundirection = glm::normalize(sunPosition);
	glm::vec4 sunAmbientLight(0.5);
	glm::vec4 sunDiffuseLight(glm::vec3(1.0f), 1.0f);
	glm::vec4 sunSpecularLight(glm::vec3(0.3f), 1.0f);
	float sunShininess = 200.0f;

	// render loop
	// -----------
	while (!glfwWindowShouldClose(window))
	{
		// per-frame time logic
		// --------------------
		double currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;
		if (running)
			simulationTime += deltaTime;

		// input
		// -----
		processInput(window);

		// compute shader 
		// --------------
		water.runWaterComputeShader(running, currentFrame, deltaTime);

		// render
		// ------
		glPolygonMode(GL_FRONT_AND_BACK, wireframe ? GL_LINE : GL_FILL);		
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glEnable(GL_DEPTH_TEST);
		// Camera & Eye
		// ------------
		glm::mat4 projectionMatrix = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
		glm::mat4 viewMatrix = camera.GetViewMatrix();
		glm::mat4 invViewProjectionMatrix = glm::inverse(projectionMatrix*viewMatrix);
		
		gBuffer.clearFinalTexture();

		// G-Buffer
		// --------
		{
			glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
			gBuffer.bindForGeomPass();
			glDepthMask(GL_TRUE); // activate writing on depth buffer
			glClear(GL_DEPTH_BUFFER_BIT);
			glEnable(GL_DEPTH_TEST);

			gBufferProgram.use();
			gBufferProgram.setMat4(gbufferProgram_ViewMatrix, viewMatrix);
			gBufferProgram.setMat4(gbufferProgram_ProjectionMatrix, projectionMatrix);

			// Render Boden
			// ------------
			{
				glm::mat4 modelMatrix(1.0);		// ModelMatrix 
				glm::mat3 normalMatrix = glm::mat3(glm::transpose(glm::inverse(modelMatrix)));	// Normal Matrix
				gBufferProgram.setMat4(gbufferProgram_ModelMatrix, modelMatrix);
				gBufferProgram.setMat3(gbufferProgram_NormalMatrix, normalMatrix);
																								
				/* Object Material */ 
				gBufferProgram.setVec4(gbufferProgram_ObjectMaterial_color, 1, 0,0,1);
				gBufferProgram.setFloat(gbufferProgram_ObjectMaterial_ambientReflection, 0.5);
				gBufferProgram.setFloat(gbufferProgram_ObjectMaterial_diffuseReflection, 0.5);
				gBufferProgram.setFloat(gbufferProgram_ObjectMaterial_specularReflection, 0.5);
				gBufferProgram.setFloat(gbufferProgram_ObjectMaterial_shininess, 1);

				gBufferProgram.setBool("ObjectMaterial.hasTexture", true);
				int textureUnitTexture = 0;
				glActiveTexture(GL_TEXTURE0 + textureUnitTexture);
				glBindTexture(GL_TEXTURE_2D, sandTexture);
				gBufferProgram.setInt("ObjectMaterialTexture", textureUnitTexture);

				ground.draw();

				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
				glBindVertexArray(0);
			}
			glUseProgram(0);
		}

		// Rendering Final Buffer
		// ----------------------
		{
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			gBuffer.bindForLightPass();

			// Rendering Lighting
			// ------------------
			{
				lightProgram.use();

				// Set Sun Light Parameter						   
				lightProgram.setMat4(lightProgram_ViewerInverseViewProjectionMatrix, invViewProjectionMatrix);
				lightProgram.setMat4(lightProgram_ViewerViewMatrix, viewMatrix);
				lightProgram.setVec3(lightProgram_Sun_Direction, sundirection);
				lightProgram.setVec4(lightProgram_Sun_AmbientColor, sunAmbientLight);
				lightProgram.setVec4(lightProgram_Sun_DiffuseColor, sunDiffuseLight);
				lightProgram.setVec4(lightProgram_Sun_SpecularColor, sunSpecularLight);
				lightProgram.setFloat(lightProgram_Sun_Shininess, sunShininess);

				glDisable(GL_DEPTH_TEST);
				glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT); // Hier muss auf den Compute Shader gewartet werden.
				renderQuad();
				glEnable(GL_DEPTH_TEST);

				glUseProgram(0);

			}

			// Wasser rendern
			// --------------
			{
				glEnable(GL_BLEND);
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
				waterProgram.use();
				waterProgram.setMat4(waterProgram_ViewMatrix, viewMatrix);
				waterProgram.setMat4(waterProgram_ProjectionMatrix, projectionMatrix);

				// Model Matrix
				glm::mat4 modelMatrix = glm::mat4(1);
				waterProgram.setMat4(waterProgram_ModelMatrix, modelMatrix);
				// Normal Matrix
				glm::mat4 normalMatrix = glm::mat3(glm::transpose(glm::inverse(modelMatrix)));
				waterProgram.setMat3(waterProgram_NormalMatrix, normalMatrix);

				waterProgram.setMat4(waterProgram_ViewerInverseViewProjectionMatrix, invViewProjectionMatrix);
				waterProgram.setMat4(waterProgram_ViewerViewMatrix, viewMatrix);

				glActiveTexture(GL_TEXTURE0 + waterProgram_EnvironmentMap);
				glBindTexture(GL_TEXTURE_2D, environmentMap);

				/*
				utils_setUniformVec3(waterProgram_Sun_Direction, &g_sun.direction);
				utils_setUniformVec4(waterProgram_Sun_AmbientColor, &g_sun.ambientLight);
				utils_setUniformVec4(waterProgram_Sun_DiffuseColor, &g_sun.diffuseLight);
				utils_setUniformVec4(waterProgram_Sun_SpecularColor, &g_sun.specularLight);
				utils_setUniformFloat(waterProgram_Sun_Shininess, g_sun.shininess);
				*/
	
				glPolygonMode(GL_FRONT_AND_BACK, wireframe ? GL_LINE : GL_FILL);
				glDepthMask(GL_TRUE);
				water.draw();
				glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
				glUseProgram(0);
				glDisable(GL_BLEND);	
			}

			// Render Sky
			// ----------
			{
				skyProgram.use();
				glEnable(GL_DEPTH_TEST);
				glDepthFunc(GL_LEQUAL);
				glCullFace(GL_FRONT);

				skyProgram.setMat4(skyProgram_ViewMatrix, viewMatrix);
				skyProgram.setMat4(skyProgram_ProjectionMatrix, projectionMatrix);
				glActiveTexture(GL_TEXTURE0 + skyProgram_EnvironmentMap);
				glBindTexture(GL_TEXTURE_2D, environmentMap);
	
				//Render 
				renderCube();
				
				glCullFace(GL_BACK);
				glDepthFunc(GL_LESS); // set depth function back to default
				glUseProgram(0);
				
			}
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		}
			
		// Rendering auf den Standard-Framebuffer
		// --------------------------------------
		{
			if (renderBuffers) {
				int halfWidth = SCR_WIDTH / 2.0;
				int halfHeight = SCR_HEIGHT / 2.0;
				

				gBuffer.bindForFinalPass();
				// Left Bottom - Final Scene
				glBlitFramebuffer(0, 0, SCR_WIDTH, SCR_HEIGHT, // SRC_RECTANGLE
								0, 0, halfWidth, halfHeight,
								GL_COLOR_BUFFER_BIT, GL_LINEAR);
				// Right Bottom - Albedo
				gBuffer.setReadBuffer(GBuffer::GBUFFER_TEXTURE_TYPE_COLOR);
				glBlitFramebuffer(0, 0, SCR_WIDTH, SCR_HEIGHT, // SRC_RECTANGLE
					halfWidth, 0, SCR_WIDTH, halfHeight,
					GL_COLOR_BUFFER_BIT, GL_LINEAR);
				// Left Top -  Normal
				gBuffer.setReadBuffer(GBuffer::GBUFFER_TEXTURE_TYPE_NORMAL);
				glBlitFramebuffer(0, 0, SCR_WIDTH, SCR_HEIGHT, // SRC_RECTANGLE
					0, halfHeight, halfWidth, SCR_HEIGHT,
					GL_COLOR_BUFFER_BIT, GL_LINEAR);
				// Right Top -  Reflection
				gBuffer.setReadBuffer(GBuffer::GBUFFER_TEXTURE_TYPE_REFLECTION);
				glBlitFramebuffer(0, 0, SCR_WIDTH, SCR_HEIGHT, // SRC_RECTANGLE
					halfWidth, halfHeight, SCR_WIDTH, SCR_HEIGHT,
					GL_COLOR_BUFFER_BIT, GL_LINEAR);

			}	
			else {
				gBuffer.bindForFinalPass();
				glBlitFramebuffer(0, 0, SCR_WIDTH, SCR_HEIGHT, // SRC_RECTANGLE
					0, 0, SCR_WIDTH, SCR_HEIGHT,
					GL_COLOR_BUFFER_BIT, GL_LINEAR);
			}			
		}

		glUseProgram(0);

		// glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
		// -------------------------------------------------------------------------------
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	// optional: de-allocate all resources once they've outlived their purpose:
	// ------------------------------------------------------------------------
	
	// glfw: terminate, clearing all previously allocated GLFW resources.
	// ------------------------------------------------------------------
	glfwTerminate();
	return 0;
}

// renderCube() renders a 1x1 3D cube in NDC.
// -------------------------------------------------
unsigned int cubeVAO = 0;
unsigned int cubeVBO = 0;
void renderCube()
{
	// initialize (if necessary)
	if (cubeVAO == 0)
	{
		float vertices[] = {
			// back face
			-1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
			 1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
			 1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 0.0f, // bottom-right         
			 1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
			-1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
			-1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 1.0f, // top-left
			// front face
			-1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
			 1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 0.0f, // bottom-right
			 1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
			 1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
			-1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f, // top-left
			-1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
			// left face
			-1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
			-1.0f,  1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-left
			-1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
			-1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
			-1.0f, -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-right
			-1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
			// right face
			 1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
			 1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
			 1.0f,  1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-right         
			 1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
			 1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
			 1.0f, -1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-left     
			// bottom face
			-1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
			 1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 1.0f, // top-left
			 1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
			 1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
			-1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f, // bottom-right
			-1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
			// top face
			-1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
			 1.0f,  1.0f , 1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
			 1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 1.0f, // top-right     
			 1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
			-1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
			-1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 0.0f  // bottom-left        
		};
		glGenVertexArrays(1, &cubeVAO);
		glGenBuffers(1, &cubeVBO);
		// fill buffer
		glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
		// link vertex attributes
		glBindVertexArray(cubeVAO);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);
	}
	// render Cube
	glBindVertexArray(cubeVAO);
	glDrawArrays(GL_TRIANGLES, 0, 36);
	glBindVertexArray(0);
}

// renderQuad() renders a 1x1 XY quad in NDC
// -----------------------------------------
unsigned int quadVAO = 0;
unsigned int quadVBO;
void renderQuad()
{
	if (quadVAO == 0)
	{
		float quadVertices[] = {
			// positions        // texture Coords
			-1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
			-1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
			 1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
			 1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
		};
		// setup plane VAO
		glGenVertexArrays(1, &quadVAO);
		glGenBuffers(1, &quadVBO);
		glBindVertexArray(quadVAO);
		glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
	}
	glBindVertexArray(quadVAO);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glBindVertexArray(0);
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window)
{	

	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);

	float cameraSpeed = 2.5f * deltaTime; // adjust accordingly

	// Wireframe
	if (glfwGetKey(window, GLFW_KEY_F1) == GLFW_PRESS)
	{
		wireframeKeyPressed = true;
	}
	if (wireframeKeyPressed && glfwGetKey(window, GLFW_KEY_F1) == GLFW_RELEASE)
	{
		wireframeKeyPressed = false;
		wireframe = !wireframe;
	}
	
	// Make Full Screen
	if (glfwGetKey(window, GLFW_KEY_F2) == GLFW_PRESS)
	{
		const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
		glfwSetWindowMonitor(window, glfwGetPrimaryMonitor(), 0, 0, mode->width, mode->height, mode->refreshRate);
	}

	// Renderbuffers
	if (glfwGetKey(window, GLFW_KEY_F3) == GLFW_PRESS)
	{
		renderBuffersKeyPressed = true;
	}
	if (renderBuffersKeyPressed && glfwGetKey(window, GLFW_KEY_F3) == GLFW_RELEASE)
	{
		renderBuffersKeyPressed = false;
		renderBuffers = !renderBuffers;
	}

	// Running
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
	{
		runningKeyPressed = true;
	}
	if (runningKeyPressed && glfwGetKey(window, GLFW_KEY_A) == GLFW_RELEASE)
	{
		runningKeyPressed = false;
		running = !running;
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