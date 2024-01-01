/* 
 *	Screen Space Ambient Occlusion (SSAO)
 *		Occlusion approximation
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
#include <random>

/* USING LEARNOPENGL CODE 1 OR OGLDEV 0*/
#define LEARNOPENGL 1
// NOTE: this is not implemented
#define OGLDEPTHBUFFEROPT 0

const static unsigned int KERNEL_SIZE = 64;

// callbacks
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
// current Mouse position 
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow *window);
/* load texture */
unsigned int loadTexture(const char *path, bool gammaCorrection);

/* DRAW */
void drawScene(Shader shader);
void renderQuad();
void renderCube();

void icon(GLFWwindow* window);

// settings
const unsigned int SCR_WIDTH = 1280;
const unsigned int SCR_HEIGHT = 720;

int shaderTypeOGLDev = 0;
bool aKeyPressed = false;

bool debug = false;
bool d_keyPressed = false;

bool blur = false;
bool b_keyPressed = false;

// camera
Camera camera(glm::vec3(0.0f, 0.0f, 5.0f));
float lastX = SCR_WIDTH / 2.0;
float lastY = SCR_HEIGHT / 2.0;
bool firstMouse = true;

// model
Model nanosuit;
unsigned int woodTexture;

// timing 
float deltaTime = 0.0f; // time between current frame and last frame
float lastFrame = 0.0f; // time of last frame

/* accelerating interpolation function */
float lerp(float a, float b, float f)
{
	// places more samples closer to its origin
	return a + f * (b - a);
}

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
#if LEARNOPENGL
	Shader shaderGeometryPass(FileSystem::getSamplePath("shader/gBufferShader.vert").c_str(), FileSystem::getSamplePath("shader/gBufferShader.frag").c_str());
	Shader shaderLightingPass(FileSystem::getSamplePath("shader/ssao.vert").c_str(), FileSystem::getSamplePath("shader/ssao_lighting.frag").c_str());
	Shader shaderSSAO(FileSystem::getSamplePath("shader/ssao.vert").c_str(), FileSystem::getSamplePath("shader/ssao.frag").c_str());
	Shader shaderSSAOBlur(FileSystem::getSamplePath("shader/ssao.vert").c_str(), FileSystem::getSamplePath("shader/ssao_blur.frag").c_str());
#else
	#if OGLDEPTHBUFFEROPT
		throw std::exception("Not Implemented");
	#else 
		Shader shaderGeometryPass(FileSystem::getSamplePath("shader/oglGPass.vert").c_str(), FileSystem::getSamplePath("shader/oglGPass.frag").c_str());
		Shader shaderLightingPass(FileSystem::getSamplePath("shader/oglLighting.vert").c_str(), FileSystem::getSamplePath("shader/oglLighting.frag").c_str());
		Shader shaderSSAO(FileSystem::getSamplePath("shader/oglssao.vert").c_str(), FileSystem::getSamplePath("shader/oglssao.frag").c_str());
		Shader shaderSSAOBlur(FileSystem::getSamplePath("shader/oglBlur.vert").c_str(), FileSystem::getSamplePath("shader/oglBlur.frag").c_str());
	#endif
#endif // LEARNOPENGL

	// load models
	// -----------
	nanosuit = Model(FileSystem::getPath("content/models/nanosuit/nanosuit.obj").c_str());

	// textures
	// --------
	woodTexture = loadTexture(FileSystem::getPath("content/images/wood.png").c_str(), false);

	// configure g-buffer framebuffer
	// ------------------------------
	GLuint gBuffer;
	glGenFramebuffers(1, &gBuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);
	GLuint gPosition, gNormal, gAlbedo;
	// position color buffer
	glGenTextures(1, &gPosition);
	glBindTexture(GL_TEXTURE_2D, gPosition);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGB, GL_FLOAT, NULL);
	// floating point, positions aren't clamped to [0.0, 1.0]
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	// ensure not accidentally oversample position/depth values in 
	// screen-space outside the texture's default coordinate region
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gPosition, 0);
	// normal color buffer
	glGenTextures(1, &gNormal);
	glBindTexture(GL_TEXTURE_2D, gNormal);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGB, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, gNormal, 0);
	// color (+ spec) color buffer
	glGenTextures(1, &gAlbedo);
	glBindTexture(GL_TEXTURE_2D, gAlbedo);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, gAlbedo, 0);
	// tell OpenGL which color attachments we'll use (of this framebuffer) for rendering
	GLuint attachments[3] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2};
	glDrawBuffers(3, attachments);
	// create and attach depth buffer (renderbuffer)
	GLuint rboDepth;
	glGenRenderbuffers(1, &rboDepth);
	glBindRenderbuffer(GL_RENDERBUFFER, rboDepth);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, SCR_WIDTH, SCR_HEIGHT);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepth);
	// finally check if framebuffer is complete
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		std::cout << "Framebuffer not complete!" << std::endl;
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	
	// create frambuffer to hold SSAO processing stage
	// -----------------------------------------------
	GLuint ssaoFBO, ssaoBlurFBO;
	glGenFramebuffers(1, &ssaoFBO);
	glGenFramebuffers(1, &ssaoBlurFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, ssaoFBO);
	GLuint ssaoColorBuffer, ssaoColorBufferBlur;
	// SSAO color buffer
	glGenTextures(1, &ssaoColorBuffer);
	glBindTexture(GL_TEXTURE_2D, ssaoColorBuffer);
	// Ambient occlusion result --> single grayscale value
	// we only need a single GL_RED Component
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGB, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ssaoColorBuffer, 0);
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		std::cout << "SSAP Framebuffer not complete!" << std::endl;
	// and blur stage
	glBindFramebuffer(GL_FRAMEBUFFER, ssaoBlurFBO);
	glGenTextures(1, &ssaoColorBufferBlur);
	glBindTexture(GL_TEXTURE_2D, ssaoColorBufferBlur);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGB, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ssaoColorBufferBlur, 0);
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		std::cout << "SSAO Blur Framebuffer not complete!" << std::endl;
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// generate sample kernel
	// ----------------------
#if LEARNOPENGL
	std::uniform_real_distribution<GLfloat> randomFloats(0.0f, 1.0f);
	// random floats between 0.0 - 1.0
	std::default_random_engine generator;
	std::vector<glm::vec3> ssaoKernel;
	for (unsigned int i = 0; i < 64; ++i) {	// 64 sample values
		glm::vec3 sample(
			randomFloats(generator) * 2.0f - 1.0f,
			randomFloats(generator) * 2.0f - 1.0f,
			randomFloats(generator) // varying z direction of the samples between 0.0 and 1.0 
			// if we'd vary the z direction between -1.0 and 1.0 as well, we'd
			// have a sphere sample kernel
		);
		// varying the the x and y direction in tangent space between -1.0 and 1.0
		sample = glm::normalize(sample);
		sample *= randomFloats(generator);
		float scale = float(i) / 64.0f;

		// scale samples s.t. they're more aligned to center of kernel / close to the origin
		scale = lerp(0.1f, 1.0f, scale * scale);
		sample *= scale;
		ssaoKernel.push_back(sample);
	}
#else 
		// ogldev
	glm::vec3 kernel[KERNEL_SIZE];
	for (unsigned int i = 0; i < KERNEL_SIZE; i++) {
		float scale = (float)i / (float)(KERNEL_SIZE);
		glm::vec3 v;
		v.x = 2.0f * (float)rand() / RAND_MAX - 1.0f;
		v.y = 2.0f * (float)rand() / RAND_MAX - 1.0f;
		v.z = 2.0f * (float)rand() / RAND_MAX - 1.0f;
		// Use an acceleration function so more points are
		// located closer to the origin
		v *= (0.1f + 0.9f * scale * scale);

		kernel[i] = v;
	}
#endif

	// generate noise texture
	// to reduce number of samples required
	// ----------------------------
#if LEARNOPENGL
	std::vector<glm::vec3> ssaoNoise; // introducing some randomness onto the sample kernels
	for (unsigned int i = 0; i < 16; i++) {	
		// 4x4 array random rotation vectors oriented around the 
		// tangent-space surface normal
		glm::vec3 noise(
			randomFloats(generator) * 2.0f - 1.0f,
			randomFloats(generator) * 2.0f - 1.0f,
			0.0f // leave z component at 0.0 so we rotate around the z axis
		);
		ssaoNoise.push_back(noise);
	}

	// reduce number of samples necessary to get good results
	// create small texture of random rotation vectors that we tile over the screen

	// create 4x4 texture that holds the random rotation vectors
	// set wrapping method to GL_REPEAT (properly tiles over the screen)
	GLuint noiseTexture;
	glGenTextures(1, &noiseTexture);
	glBindTexture(GL_TEXTURE_2D, noiseTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, 4, 4, 0, GL_RGB, GL_FLOAT, &ssaoNoise[0]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT); // repeat texNoise all over the screen
#endif

	// lighting info
	// -------------
	glm::vec3 lightDir = glm::vec3(0.0f, -1.0f, 0.0f);

	glm::vec3 lightPos = glm::vec3(2.0f, 4.0f, -2.0f);
	glm::vec3 lightColor = glm::vec3(1.0f, 1.0f, 1.0f);
	//glm::vec3 lightColor = glm::vec3(0.2f, 0.2f, 0.7f);
	float lightAmbientIntensity = 0.5f;
	float lightDiffuseIntensity = 0.6f;

	// shader configuration
	// --------------------
#if LEARNOPENGL
	shaderLightingPass.use();
	shaderLightingPass.setInt("gPosition", 0);
	shaderLightingPass.setInt("gNormal", 1);
	shaderLightingPass.setInt("gAlbedo", 2);
	shaderLightingPass.setInt("ssao", 3);
	shaderSSAO.use();
	shaderSSAO.setInt("gPosition", 0);
	shaderSSAO.setInt("gNormal", 1);
	shaderSSAO.setInt("texNoise", 2);
	shaderSSAOBlur.use();
	shaderSSAOBlur.setInt("ssaoInput", 0);
#else 
	shaderSSAO.use();
	shaderSSAO.setInt("gPositionMap", 0);
	shaderSSAOBlur.use();
	shaderSSAOBlur.setInt("gColorMap", 0);
	shaderLightingPass.use();
	shaderLightingPass.setInt("gAOMap", 3);
#endif

	// render loop
	// -----------
	while (!glfwWindowShouldClose(window))
	{
		// Set frame time
		GLfloat currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		// Check and call events
		processInput(window);

		// render
		// ------
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// camera
		// ------
		float fov = glm::radians(camera.Zoom);
		float zNear = 0.1f;
		float zFar = 500.0f;
		float width = (float)SCR_WIDTH;
		float height = (float)SCR_HEIGHT;

		glm::mat4 projection = glm::perspective(fov, width / height, zNear, zFar);
		glm::mat4 view = camera.GetViewMatrix();

#if LEARNOPENGL
		// LEARNOPENGL SOURCE
		// 1. geometry pass: render scene's geometry/color data into gbuffer
		// -----------------------------------------------------------------
		glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			shaderGeometryPass.use();
			shaderGeometryPass.setMat4("projection", projection);
			shaderGeometryPass.setMat4("view", view);
			drawScene(shaderGeometryPass);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		
		// 2. generate SSAO texture
	    // ------------------------
		glBindFramebuffer(GL_FRAMEBUFFER, ssaoFBO);
			glClear(GL_COLOR_BUFFER_BIT);
			shaderSSAO.use();
			// Send kernel + rotation 
			for (unsigned int i = 0; i < 64; ++i)
				shaderSSAO.setVec3("samples[" + std::to_string(i) + "]", ssaoKernel[i]);
			shaderSSAO.setMat4("projection", projection);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, gPosition);
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, gNormal);
			glActiveTexture(GL_TEXTURE2);
			glBindTexture(GL_TEXTURE_2D, noiseTexture);
			renderQuad();
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		// 3. blur SSAO texture to remove noise
		// ------------------------------------
		if (blur) {
			glBindFramebuffer(GL_FRAMEBUFFER, ssaoBlurFBO);
			glClear(GL_COLOR_BUFFER_BIT);
			shaderSSAOBlur.use();
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, ssaoColorBuffer);
			renderQuad();
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		}

		if (debug) {
			// debug mode only show SSAO texutre
			if (blur) {
				glBindFramebuffer(GL_READ_FRAMEBUFFER, ssaoBlurFBO);
			} 
			else {
				glBindFramebuffer(GL_READ_FRAMEBUFFER, ssaoFBO);
			}
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
			glBlitFramebuffer(0, 0, SCR_WIDTH, SCR_HEIGHT, 0, 0, SCR_WIDTH, SCR_HEIGHT, GL_COLOR_BUFFER_BIT, GL_LINEAR);
		}
		else {
			// 4. lighting pass: traditional deferred Blinn-Phong lighting with added screen-space ambient occlusion
			// -----------------------------------------------------------------------------------------------------
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			shaderLightingPass.use();
			// send light relevant uniforms
			glm::vec3 lightPosView = glm::vec3(camera.GetViewMatrix() * glm::vec4(lightPos, 1.0));
			shaderLightingPass.setVec3("light.Position", lightPosView);
			shaderLightingPass.setVec3("light.Color", lightColor);
			// Update attenuation parameters
			const float constant = 1.0;
			// note that we don't send this to the shader, we assume it is always 1.0 (in our case)
			const float linear = 0.09;
			const float quadratic = 0.032;
			shaderLightingPass.setFloat("light.Linear", linear);
			shaderLightingPass.setFloat("light.Quadratic", quadratic);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, gPosition);
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, gNormal);
			glActiveTexture(GL_TEXTURE2);
			glBindTexture(GL_TEXTURE_2D, gAlbedo);
			glActiveTexture(GL_TEXTURE3); // add extra SSAO texture to lighting pass
			if (blur) {
				glBindTexture(GL_TEXTURE_2D, ssaoColorBufferBlur);
			} else {
				glBindTexture(GL_TEXTURE_2D, ssaoColorBuffer);
			}
			renderQuad();
		}
#else 
	#if OGLDEPTHBUFFEROPT
		throw std::exception("Not Implemented");
	#else 
		
			// Geometry Pass
			{
				glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);
					glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
					shaderGeometryPass.use();
					shaderGeometryPass.setMat4("projection", projection);
					shaderGeometryPass.setMat4("view", view);
					drawScene(shaderGeometryPass);
				glBindFramebuffer(GL_FRAMEBUFFER, 0);
			}
			
			// SSAO Pass
			{
				glBindFramebuffer(GL_FRAMEBUFFER, ssaoFBO);
					glClear(GL_COLOR_BUFFER_BIT);
					shaderSSAO.use();
					for (unsigned int i = 0; i < KERNEL_SIZE; ++i)
						shaderSSAO.setVec3("gKernel[" + std::to_string(i) + "]", kernel[i]);
					shaderSSAO.setMat4("projection", projection);
					shaderSSAO.setFloat("gSampleRad", 1.5f);
					shaderSSAO.setInt("gPositionMap", 0);
					glActiveTexture(GL_TEXTURE0);
					glBindTexture(GL_TEXTURE_2D, gPosition);

					renderQuad(); // Render to a Full Screen Quad
				glBindFramebuffer(GL_FRAMEBUFFER, 0);
			}
			
			// Blur Pass
			if (blur) {
				glBindFramebuffer(GL_FRAMEBUFFER, ssaoBlurFBO);
					glClear(GL_COLOR_BUFFER_BIT);
					shaderSSAOBlur.use();
					glActiveTexture(GL_TEXTURE0);
					glBindTexture(GL_TEXTURE_2D, ssaoColorBuffer);
					renderQuad(); // Render to a Full Screen Quad
				glBindFramebuffer(GL_FRAMEBUFFER, 0);

			}
			if (debug) {
				if (blur) {
					glBindFramebuffer(GL_READ_FRAMEBUFFER, ssaoBlurFBO);
				}
				else {
					glBindFramebuffer(GL_READ_FRAMEBUFFER, ssaoFBO);
				}
				glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
				glBlitFramebuffer(0, 0, SCR_WIDTH, SCR_HEIGHT, 0, 0, SCR_WIDTH, SCR_HEIGHT, GL_COLOR_BUFFER_BIT, GL_LINEAR);
			}
			else
			{ 
				//	Lighting Pass
				glBindFramebuffer(GL_FRAMEBUFFER, 0);
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

				shaderLightingPass.use();
				shaderLightingPass.setMat4("projection", projection);
				shaderLightingPass.setMat4("view", view);
				
				shaderLightingPass.setVec2("gScreenSize", glm::vec2(SCR_WIDTH, SCR_HEIGHT));
				shaderLightingPass.setInt("gShaderType", shaderTypeOGLDev);
				shaderLightingPass.setVec3("gEyeWorldPos", camera.Position);

				/* LIGHTING*/
				shaderLightingPass.setVec3("gDirectionalLight.Base.Color", lightColor);
				shaderLightingPass.setVec3("gDirectionalLight.Direction", lightDir);
				shaderLightingPass.setFloat("gDirectionalLight.Base.AmbientIntensity", lightAmbientIntensity);
				shaderLightingPass.setFloat("gDirectionalLight.Base.DiffuseIntensity", lightDiffuseIntensity);

				glActiveTexture(GL_TEXTURE3); // add extra SSAO texture to lighting pass
				if (blur) {
					glBindTexture(GL_TEXTURE_2D, ssaoColorBufferBlur);
				}
				else {
					glBindTexture(GL_TEXTURE_2D, ssaoColorBuffer);
				}

				drawScene(shaderLightingPass);
			}

	#endif
#endif
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


void drawScene(Shader shader) {
	glm::mat4 model;
	shader.use();
	shader.setInt("material.texture_diffuse1", 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, woodTexture);

	// room cube
	glEnable(GL_CULL_FACE);		// change cull methode 
	glCullFace(GL_FRONT);
	model = glm::mat4(1.0);
	model = glm::translate(model, glm::vec3(0.0, 7.0f, 0.0f));
	model = glm::scale(model, glm::vec3(7.5f, 7.5f, 7.5f));
	shader.setMat4("model", model);
	shader.setBool("invertedNormals", 1);
	// invert normals as we're inside the cube
	renderCube();
	// disable invert normals
	shader.setBool("invertedNormals", 0);

	glDisable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	// nanosuit model on the floor
	model = glm::mat4(1.0);
	model = glm::translate(model, glm::vec3(0.0f, 0.0f, 5.0));
	model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0, 0.0, 0.0));
	model = glm::scale(model, glm::vec3(0.5f));
	shader.setMat4("model", model);
	nanosuit.Draw(shader);
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

	if (glfwGetKey(window, GLFW_KEY_F2) == GLFW_PRESS)
	{
		const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
		glfwSetWindowMonitor(window, glfwGetPrimaryMonitor(), 0, 0, mode->width, mode->height, mode->refreshRate);
	}

	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
		aKeyPressed = true;
	}

	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_RELEASE && aKeyPressed) {
		shaderTypeOGLDev++;
		shaderTypeOGLDev %= 3;
		aKeyPressed = false;
	}

	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
		d_keyPressed = true;
	}

	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_RELEASE && d_keyPressed) {
		debug = !debug;
		d_keyPressed = false;
	}

	if (glfwGetKey(window, GLFW_KEY_B) == GLFW_PRESS) {
		b_keyPressed = true;
	}

	if (glfwGetKey(window, GLFW_KEY_B) == GLFW_RELEASE && b_keyPressed) {
		blur = !blur;
		b_keyPressed = false;
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