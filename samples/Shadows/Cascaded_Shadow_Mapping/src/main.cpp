/* 
 *	Cascaded Shadow Mapping
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
#include "modules/light.h"
#include "modules/material.h"
#include "modules/filesystem.h"

#include <iostream>
#include <random>

#define PETERPANING

// implementation 
// 0 learnopengl based   this implementation useses a geometry shader
// 1 ogldev base         this implementation renders the scene mutliple times (avoids the use of the geo shader)
#define IMPLEMENTATION 1


// callbacks
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
// current Mouse position 
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow *window);
void drawPlane(void);
/* load texture */
unsigned int loadTexture(const char *path, bool gammaCorrection);
void renderScene(const Shader &shader);
void renderCube();
void renderQuad();

std::vector<glm::mat4> getLightSpaceMatrices();
std::vector<glm::vec4> getFrustumCornersWorldSpace(const glm::mat4& projview);
void drawCascadeVolumeVisualizers(const std::vector<glm::mat4>& lightMatrices, Shader* shader);

void icon(GLFWwindow* window);

// settings
const unsigned int SCR_WIDTH = 1280;
const unsigned int SCR_HEIGHT = 720;

// framebuffer size
int fb_width;
int fb_height;

// camera
Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
float lastX = SCR_WIDTH / 2.0;
float lastY = SCR_HEIGHT / 2.0;
bool firstMouse = true;

float cameraNearPlane = 0.1f;
float cameraFarPlane = 500.0f;

// timing 
float deltaTime = 0.0f; // time between current frame and last frame
float lastFrame = 0.0f; // time of last frame

std::vector<float> shadowCascadeLevels{ cameraFarPlane / 50.0f, cameraFarPlane / 25.0f, cameraFarPlane / 10.0f, cameraFarPlane / 2.0f };
int debugLayer = 0;

// meshes
unsigned int planeVAO;

// lighting info
// -------------
const glm::vec3 lightDir = glm::normalize(glm::vec3(20.0f, 50, 20.0f));	// NOTE: 1 hardcoded light!
unsigned int lightFBO;
#if IMPLEMENTATION == 0
	unsigned int lightDepthMaps;
#elif IMPLEMENTATION == 1
	unsigned int lightDepthMaps[4];
#endif
constexpr unsigned int depthMapResolution = 4096;

bool showQuad = false;

std::random_device device;
std::mt19937 generator = std::mt19937(device());

std::vector<glm::mat4> lightMatricesCache;

int main()
{
	// glfw: initialize and configure
	// ------------------------------
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	//glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

	#ifdef __APPLE__
		glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // uncomment this statement to fix compilation on OS X
	#endif

	// glfw window creation
	// --------------------
	GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Shadows", NULL, NULL);
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

	glfwGetFramebufferSize(window, &fb_width, &fb_height);

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
#if IMPLEMENTATION == 0
	Shader shader(FileSystem::getSamplePath("shader/shadow_mapping.vert").c_str(), FileSystem::getSamplePath("shader/shadow_mapping.frag").c_str());
	Shader simpleDepthShader(FileSystem::getSamplePath("shader/shadow_mapping_depth.vert").c_str(), FileSystem::getSamplePath("shader/shadow_mapping_depth.frag").c_str(), FileSystem::getPath("shader/shadow_mapping_depth.geom").c_str());
	Shader debugDepthQuad(FileSystem::getSamplePath("shader/debug_quad.vert").c_str(), FileSystem::getSamplePath("shader/debug_quad_depth.frag").c_str());
	Shader debugCascadeShader(FileSystem::getSamplePath("shader/debug_cascade.vert").c_str(), FileSystem::getSamplePath("shader/debug_cascade.frag").c_str());
#elif IMPLEMENTATION == 1
	Shader shader(FileSystem::getSamplePath("shader/lighting.vert").c_str(), FileSystem::getSamplePath("shader/lighting.frag").c_str());
	Shader simpleDepthShader(FileSystem::getSamplePath("shader/csm.vert").c_str(), FileSystem::getSamplePath("shader/csm.frag").c_str());
	Shader debugDepthQuad(FileSystem::getSamplePath("shader/debug_quad.vert").c_str(), FileSystem::getSamplePath("shader/debug_quad_depth_multi_tex.frag").c_str());
	Shader debugCascadeShader(FileSystem::getSamplePath("shader/debug_cascade.vert").c_str(), FileSystem::getSamplePath("shader/debug_cascade.frag").c_str());
#endif
	// set up vertex data (and buffer(s)) and configure vertex attributes
	// ------------------------------------------------------------------
	float planeVertices[] = {
		// positions            // normals         // texcoords
		 25.0f, -2.0f,  25.0f,  0.0f, 1.0f, 0.0f,  25.0f,  0.0f,
		-25.0f, -2.0f,  25.0f,  0.0f, 1.0f, 0.0f,   0.0f,  0.0f,
		-25.0f, -2.0f, -25.0f,  0.0f, 1.0f, 0.0f,   0.0f, 25.0f,
		 25.0f, -2.0f,  25.0f,  0.0f, 1.0f, 0.0f,  25.0f,  0.0f,
		-25.0f, -2.0f, -25.0f,  0.0f, 1.0f, 0.0f,   0.0f, 25.0f,
		 25.0f, -2.0f, -25.0f,  0.0f, 1.0f, 0.0f,  25.0f, 25.0f
	};

	// Setup cube VAO
	GLuint planeVBO;
	glGenVertexArrays(1, &planeVAO);
	glGenBuffers(1, &planeVBO);
	glBindVertexArray(planeVAO);
	glBindBuffer(GL_ARRAY_BUFFER, planeVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(planeVertices), &planeVertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);	// positions
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));	// normals
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));	// textcoords
	glBindVertexArray(0);

	// load textures
	// -------------
	unsigned int woodTexture = loadTexture(FileSystem::getPath("content/images/wood.png").c_str(), GL_FALSE);

	// configure light FBO
    // -----------------------
#if IMPLEMENTATION == 0
	glGenFramebuffers(1, &lightFBO);

	glGenTextures(1, &lightDepthMaps);
	glBindTexture(GL_TEXTURE_2D_ARRAY, lightDepthMaps);
	// create 2d texture arrays with teextures of the same dimensions using glTexImage3D
	glTexImage3D(
		GL_TEXTURE_2D_ARRAY, 0, GL_DEPTH_COMPONENT32F, depthMapResolution, depthMapResolution, int(shadowCascadeLevels.size()) + 1,
		0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);

	// Note: use of GL_TEXTURE_2D_ARRAY
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

	constexpr float bordercolor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	glTexParameterfv(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_BORDER_COLOR, bordercolor);

	glBindFramebuffer(GL_FRAMEBUFFER, lightFBO);
	// nutzen von framebufferTexture anstelle von frameBufferTexture2D
	glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, lightDepthMaps, 0);
	// Disable writes to the color buffer
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);

	int status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (status != GL_FRAMEBUFFER_COMPLETE)
	{
		std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!";
		throw 0;
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
#elif IMPLEMENTATION == 1
	// CascadedShadowMapFBO
	glGenFramebuffers(1, &lightFBO);

	glGenTextures(shadowCascadeLevels.size(), lightDepthMaps);

	for (GLuint i = 0; i < shadowCascadeLevels.size(); i++) {
		glBindTexture(GL_TEXTURE_2D, lightDepthMaps[i]);

		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, depthMapResolution, depthMapResolution, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	}

	glBindFramebuffer(GL_FRAMEBUFFER, lightFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, lightDepthMaps[0], 0);
	// Disable writes to the color buffer
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);

	int status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (status != GL_FRAMEBUFFER_COMPLETE)
	{
		std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!";
		throw 0;
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
#endif
	// configure UBO
	// --------------------
	unsigned int matricesUBO;
	glGenBuffers(1, &matricesUBO);
	glBindBuffer(GL_UNIFORM_BUFFER, matricesUBO);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(glm::mat4x4) * 16, nullptr, GL_STATIC_DRAW);	// 16 is an arbitrary number to reserve
	glBindBufferBase(GL_UNIFORM_BUFFER, 0, matricesUBO);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);


	// shader configuration
	// --------------------
	shader.use();
	shader.setInt("diffuseTexture", 0);
#if IMPLEMENTATION == 0
	shader.setInt("shadowMap", 1);
#elif IMPLEMENTATION == 1
	shader.setInt("ShadowMap[0]", 1);
	shader.setInt("ShadowMap[1]", 2);
	shader.setInt("ShadowMap[2]", 3);
	shader.setInt("ShadowMap[3]", 4);
#endif
	debugDepthQuad.use();
	debugDepthQuad.setInt("depthMap", 0);


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

		// change light position over time
		//lightPos.x = sin(glfwGetTime()) * 3.0f;
		//lightPos.z = cos(glfwGetTime()) * 2.0f;
		//lightPos.y = 5.0 + cos(glfwGetTime()) * 1.0f;

		// render
		// ------
		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// 0. UBO setup
		// unfied buffer object with light space matrices
		const auto lightMatrices = getLightSpaceMatrices();
		glBindBuffer(GL_UNIFORM_BUFFER, matricesUBO);
		for (size_t i = 0; i < lightMatrices.size(); ++i)
		{
			glBufferSubData(GL_UNIFORM_BUFFER, i * sizeof(glm::mat4x4), sizeof(glm::mat4x4), &lightMatrices[i]);
		}
		glBindBuffer(GL_UNIFORM_BUFFER, 0);

		// 1. render depth of scene to texture (from light's perspective)
		// --------------------------------------------------------------
		// orthographic proj. matrix for light src, since all light rays are parallel
		// make sure size of the projection frustum correctly contains the objects to be render in the depth map
		// note that if you use a perspective projection matrix you'll have to change the light position as the current light position isn't enough to reflect the whole scene
		// lightProjection = glm::perspective(glm::radians(45.0f), (GLfloat)SHADOW_WIDTH / (GLfloat)SHADOW_HEIGHT, near_plane, far_plane); 
		
		// render scene from light's point of view
		simpleDepthShader.use();

		glBindFramebuffer(GL_FRAMEBUFFER, lightFBO);
			glViewport(0, 0, depthMapResolution, depthMapResolution);
#if IMPLEMENTATION == 0		
			glClear(GL_DEPTH_BUFFER_BIT);
			// will cause every obj that get's rendered to not get cut out 
			// artifacts where some cascades cut out some geometry will disappear
			//glEnable(GL_DEPTH_CLAMP);
#ifdef PETERPANING
			glEnable(GL_CULL_FACE);
			glCullFace(GL_FRONT);	// peter panning
#endif
			renderScene(simpleDepthShader);		// render scene into depth map
#ifdef PETERPANING
			glCullFace(GL_BACK); // don't forget to reset original culling face
			glDisable(GL_CULL_FACE);
#endif
			//glDisable(GL_DEPTH_CLAMP);
#elif IMPLEMENTATION == 1
		for (GLuint i = 0; i < shadowCascadeLevels.size(); i++) {
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, lightDepthMaps[i], 0);
			glClear(GL_DEPTH_BUFFER_BIT);

			simpleDepthShader.setInt("layer", i);
#ifdef PETERPANING
			glEnable(GL_CULL_FACE);
			glCullFace(GL_FRONT);	// peter panning
#endif
			renderScene(simpleDepthShader);		// render scene into depth map
#ifdef PETERPANING
			glCullFace(GL_BACK); // don't forget to reset original culling face
			glDisable(GL_CULL_FACE);
#endif
		}
#endif
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		// reset viewport
		glViewport(0, 0, fb_width, fb_height);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// 2. render scene as normal using the generated depth/shadow map  
		// --------------------------------------------------------------
		glViewport(0, 0, fb_width, fb_height);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		shader.use();
		const glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)fb_width / (float)fb_height, cameraNearPlane, cameraFarPlane);
		const glm::mat4 view = camera.GetViewMatrix();
		shader.setMat4("projection", projection);
		shader.setMat4("view", view);
		// set light uniforms
		shader.setVec3("viewPos", camera.Position);
		shader.setVec3("lightDir", lightDir);
		shader.setFloat("farPlane", cameraFarPlane);
		shader.setInt("cascadeCount", shadowCascadeLevels.size());

		for (size_t i = 0; i < shadowCascadeLevels.size(); ++i)
		{
			shader.setFloat("cascadePlaneDistances[" + std::to_string(i) + "]", shadowCascadeLevels[i]);
		}
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, woodTexture);
		
#if IMPLEMENTATION == 0
		// NOTE: binding GL_TEXTURE_2D_ARRAY
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D_ARRAY, lightDepthMaps);
#elif IMPLEMENTATION == 1
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, lightDepthMaps[0]);
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, lightDepthMaps[1]);
		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_2D, lightDepthMaps[2]);
		glActiveTexture(GL_TEXTURE4);
		glBindTexture(GL_TEXTURE_2D, lightDepthMaps[3]);
#endif
		renderScene(shader);

		if (lightMatricesCache.size() != 0)
		{
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			debugCascadeShader.use();
			debugCascadeShader.setMat4("projection", projection);
			debugCascadeShader.setMat4("view", view);
			// draw light matrices volumes based on cache
			// C freezes the camera pos for the light frustums
			drawCascadeVolumeVisualizers(lightMatricesCache, &debugCascadeShader);
			glDisable(GL_BLEND);
		}

		// DEBUG
		// render Depth map to quad for visual debugging
		// ---------------------------------------------
		debugDepthQuad.use();
		glActiveTexture(GL_TEXTURE0);
#if IMPLEMENTATION == 0
		glBindTexture(GL_TEXTURE_2D_ARRAY, lightDepthMaps);
#elif IMPLEMENTATION == 1
		glBindTexture(GL_TEXTURE_2D, lightDepthMaps[debugLayer]);
		//glBindTexture(GL_TEXTURE_2D, woodTexture);
#endif
		if (showQuad)
		{
			renderQuad();
		}
		
		// glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
		// -------------------------------------------------------------------------------
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	// optional: de-allocate all resources once they've outlived their purpose:
	// ------------------------------------------------------------------------
	glDeleteVertexArrays(1, &planeVAO);
	glDeleteBuffers(1, &planeVBO);
	
	// glfw: terminate, clearing all previously allocated GLFW resources.
	// ------------------------------------------------------------------
	glfwTerminate();
	return EXIT_SUCCESS;
}

// renders the 3D scene
// --------------------
void renderScene(const Shader &shader)
{
	// floor
	glm::mat4 model = glm::mat4(1.0f);
	shader.setMat4("model", model);
	glBindVertexArray(planeVAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);

	static std::vector<glm::mat4> modelMatrices;
	if (modelMatrices.size() == 0)
	{
		for (int i = 0; i < 10; ++i)
		{
			static std::uniform_real_distribution<float> offsetDistribution = std::uniform_real_distribution<float>(-10, 10);
			static std::uniform_real_distribution<float> scaleDistribution = std::uniform_real_distribution<float>(1.0, 2.0);
			static std::uniform_real_distribution<float> rotationDistribution = std::uniform_real_distribution<float>(0, 180);

			auto model = glm::mat4(1.0f);
			model = glm::translate(model, glm::vec3(offsetDistribution(generator), offsetDistribution(generator) + 10.0f, offsetDistribution(generator)));
			model = glm::rotate(model, glm::radians(rotationDistribution(generator)), glm::normalize(glm::vec3(1.0, 0.0, 1.0)));
			model = glm::scale(model, glm::vec3(scaleDistribution(generator)));
			modelMatrices.push_back(model);
		}
	}

	for (const auto& model : modelMatrices)
	{
		shader.setMat4("model", model);
		renderCube();
	}
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

std::vector<GLuint> visualizerVAOs;
std::vector<GLuint> visualizerVBOs;
std::vector<GLuint> visualizerEBOs;
void drawCascadeVolumeVisualizers(const std::vector<glm::mat4>& lightMatrices, Shader* shader)
{
	visualizerVAOs.resize(8);
	visualizerEBOs.resize(8);
	visualizerVBOs.resize(8);

	const GLuint indices[] = {
		0, 2, 3,
		0, 3, 1,
		4, 6, 2,
		4, 2, 0,
		5, 7, 6,
		5, 6, 4,
		1, 3, 7,
		1, 7, 5,
		6, 7, 3,
		6, 3, 2,
		1, 5, 4,
		0, 1, 4
	};

	const glm::vec4 colors[] = {
		{1.0, 0.0, 0.0, 0.5f},
		{0.0, 1.0, 0.0, 0.5f},
		{0.0, 0.0, 1.0, 0.5f},
	};

	for (int i = 0; i < lightMatrices.size(); ++i)
	{
		const auto corners = getFrustumCornersWorldSpace(lightMatrices[i]);
		std::vector<glm::vec3> vec3s;
		for (const auto& v : corners)
		{
			vec3s.push_back(glm::vec3(v));
		}

		glGenVertexArrays(1, &visualizerVAOs[i]);
		glGenBuffers(1, &visualizerVBOs[i]);
		glGenBuffers(1, &visualizerEBOs[i]);

		glBindVertexArray(visualizerVAOs[i]);

		glBindBuffer(GL_ARRAY_BUFFER, visualizerVBOs[i]);
		glBufferData(GL_ARRAY_BUFFER, vec3s.size() * sizeof(glm::vec3), &vec3s[0], GL_STATIC_DRAW);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, visualizerEBOs[i]);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, 36 * sizeof(GLuint), &indices[0], GL_STATIC_DRAW);

		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);

		glBindVertexArray(visualizerVAOs[i]);
		shader->setVec4("color", colors[i % 3]);
		glDrawElements(GL_TRIANGLES, GLsizei(36), GL_UNSIGNED_INT, 0);

		glDeleteBuffers(1, &visualizerVBOs[i]);
		glDeleteBuffers(1, &visualizerEBOs[i]);
		glDeleteVertexArrays(1, &visualizerVAOs[i]);

		glBindVertexArray(0);
	}

	visualizerVAOs.clear();
	visualizerEBOs.clear();
	visualizerVBOs.clear();
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

	static int fPress = GLFW_RELEASE;
	if (glfwGetKey(window, GLFW_KEY_F) == GLFW_RELEASE && fPress == GLFW_PRESS)
	{
		showQuad = !showQuad;
	}
	fPress = glfwGetKey(window, GLFW_KEY_F);

	static int plusPress = GLFW_RELEASE;
	if (glfwGetKey(window, GLFW_KEY_N) == GLFW_RELEASE && plusPress == GLFW_PRESS)
	{
		debugLayer++;
		if (debugLayer > shadowCascadeLevels.size())
		{
			debugLayer = 0;
		}
	}
	plusPress = glfwGetKey(window, GLFW_KEY_N);

	static int cPress = GLFW_RELEASE;
	if (glfwGetKey(window, GLFW_KEY_C) == GLFW_RELEASE && cPress == GLFW_PRESS)
	{
		lightMatricesCache = getLightSpaceMatrices();
	}
	cPress = glfwGetKey(window, GLFW_KEY_C);

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
	fb_width = width;
	fb_height = height;
	glViewport(0, 0, width, height);
}

// utility function for loading a 2D texture from file
// ---------------------------------------------------
unsigned int loadTexture(char const *path, bool gammaCorrection)
{
	/*
	 * Careful: specular-maps anf normal-maps are almost always in lin. space!!! Using SRGB will break down the lightning
	 */
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
		else if (nrComponents == 3) {
			internalFormat = (gammaCorrection) ? GL_SRGB : GL_RGB;	// GL_SRGB => OpenGL will correct image to linear color space
			dataFormat = GL_RGB;
		}
		else if (nrComponents == 4) {
			internalFormat = (gammaCorrection) ? GL_SRGB_ALPHA : GL_RGBA; // GL_SRGB_ALPHA => OpenGL will correct image to linear color space
			dataFormat = GL_RGBA;
		}

		glBindTexture(GL_TEXTURE_2D, textureID);
		glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, dataFormat, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, (internalFormat == GL_RGBA || internalFormat == GL_SRGB_ALPHA) ? GL_CLAMP_TO_EDGE : GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, (internalFormat == GL_RGBA || internalFormat == GL_SRGB_ALPHA) ? GL_CLAMP_TO_EDGE : GL_REPEAT);
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

std::vector<glm::vec4> getFrustumCornersWorldSpace(const glm::mat4& projview)
{
	// calculate the inverse view & projection matrix
	const auto inv = glm::inverse(projview);

	std::vector<glm::vec4> frustumCorners;
	for (unsigned int x = 0; x < 2; ++x)
	{
		for (unsigned int y = 0; y < 2; ++y)
		{
			for (unsigned int z = 0; z < 2; ++z)
			{
				// corner points of the frsutum
				// bring x, y, z from [0,1] to [-1,1] 
				const glm::vec4 pt = inv * glm::vec4(2.0f * x - 1.0f, 2.0f * y - 1.0f, 2.0f * z - 1.0f, 1.0f);
				frustumCorners.push_back(pt / pt.w);
			}
		}
	}

	return frustumCorners;
}

std::vector<glm::vec4> getFrustumCornersWorldSpace(const glm::mat4& proj, const glm::mat4& view)
{
	return getFrustumCornersWorldSpace(proj * view);
}

/// <summary>
/// Creates a light space matrix.
/// </summary>
/// <param name="nearPlane">The near plane of current frustum.</param>
/// <param name="farPlane">The far plane of current frustum.</param>
/// <returns></returns>
glm::mat4 getLightSpaceMatrix(const float nearPlane, const float farPlane)
{
	// calc perspective matrix
	const auto proj = glm::perspective(
		glm::radians(camera.Zoom),	// cameras aspect ration & fov
		(float)fb_width / (float)fb_height,
		nearPlane,
		farPlane);

	const auto corners = getFrustumCornersWorldSpace(proj, camera.GetViewMatrix());

	glm::vec3 center = glm::vec3(0, 0, 0);
	for (const auto& v : corners)
	{
		center += glm::vec3(v);	// calc center of the frustum
	}
	center /= corners.size();

	// directional light
	const auto lightView = glm::lookAt(center + lightDir, center, glm::vec3(0.0f, 1.0f, 0.0f));

	// obtain min and maximum positions of the frustum in light space
#if IMPLEMENTATION == 0
	float minX = std::numeric_limits<float>::max();
	float maxX = std::numeric_limits<float>::lowest();
	float minY = std::numeric_limits<float>::max();
	float maxY = std::numeric_limits<float>::lowest();
	float minZ = std::numeric_limits<float>::max();
	float maxZ = std::numeric_limits<float>::lowest();
#else IMPLEMENTATION == 1
	float minX = std::numeric_limits<float>::max();
	float maxX = std::numeric_limits<float>::min();
	float minY = std::numeric_limits<float>::max();
	float maxY = std::numeric_limits<float>::min();
	float minZ = std::numeric_limits<float>::max();
	float maxZ = std::numeric_limits<float>::min();
#endif
	for (const auto& v : corners)
	{
		// bring vertices of the corners to lightspace
		// to light space
		const auto trf = lightView * v;
		minX = std::min(minX, trf.x);
		maxX = std::max(maxX, trf.x);
		minY = std::min(minY, trf.y);
		maxY = std::max(maxY, trf.y);
		minZ = std::min(minZ, trf.z);
		maxZ = std::max(maxZ, trf.z);
	}

	// Tune this parameter according to the scene
	// multiply or divide to include geometry which is behind or in front of the frsutum in camera space
	// increases the space covered by the near and far pane of the light frustum
	constexpr float zMult = 10.0f;
	if (minZ < 0)
	{
		minZ *= zMult;
	}
	else
	{
		minZ /= zMult;
	}
	if (maxZ < 0)
	{
		maxZ /= zMult;
	}
	else
	{
		maxZ *= zMult;
	}

	// directional light --> matrix will be an orthographic project matrix
	const glm::mat4 lightProjection = glm::ortho(minX, maxX, minY, maxY, minZ, maxZ);
	// user comment
	//const glm::mat4 lightProjection = glm::ortho(minX, maxX, minY, maxY, -1 * maxZ, -1 * minZ);
	return lightProjection * lightView;	// need to do this procedure for every furstum in the cascade
}

std::vector<glm::mat4> getLightSpaceMatrices()
{
	std::vector<glm::mat4> ret;
	for (size_t i = 0; i < shadowCascadeLevels.size() + 1; ++i)
	{
		if (i == 0)
		{
			ret.push_back(getLightSpaceMatrix(cameraNearPlane, shadowCascadeLevels[i]));
		}
		else if (i < shadowCascadeLevels.size())
		{
			ret.push_back(getLightSpaceMatrix(shadowCascadeLevels[i - 1], shadowCascadeLevels[i]));
		}
		else
		{
			ret.push_back(getLightSpaceMatrix(shadowCascadeLevels[i - 1], cameraFarPlane));
		}
	}
	return ret;
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