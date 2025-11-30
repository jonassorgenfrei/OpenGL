/*
 * Stencil Shadow Volumes (depth-fail / Carmack's reverse)
 * Renders a rotating cube casting a stencil shadow onto a plane.
 */

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "modules/shader_m.h"
#include "modules/camera.h"
#include "modules/filesystem.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <iostream>
#include <limits>
#include <unordered_map>
#include <vector>

// callbacks
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow *window);

void renderCube();
void renderVolumeCube();
void renderPlane();
void renderSphere();

void icon(GLFWwindow* window);

struct EdgeKey {
	unsigned int a;
	unsigned int b;

	bool operator==(const EdgeKey& other) const
	{
		return a == other.a && b == other.b;
	}
};

struct EdgeKeyHash {
	size_t operator()(const EdgeKey& key) const
	{
		return (static_cast<size_t>(key.a) << 32) ^ static_cast<size_t>(key.b);
	}
};

std::vector<unsigned int> buildAdjacency(const std::vector<unsigned int>& baseIndices);

// settings
const unsigned int SCR_WIDTH = 1280;
const unsigned int SCR_HEIGHT = 1024;

bool w_pressed = false;
bool l_pressed = false;
bool v_pressed = false;

bool visLight = false;
bool enable_shadows = true;
bool showVolumes = false;

bool space_pressed = false;
bool wireframe = false;

// camera
Camera camera(glm::vec3(0.0f, 1.0f, 7.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// timing 
float deltaTime = 0.0f; // time between current frame and last frame
float lastFrame = 0.0f; // time of last frame

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
		glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	#endif

	// glfw window creation
	// --------------------
	GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Stencil Shadow Volumes", NULL, NULL);
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
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glDepthFunc(GL_LEQUAL);

	// build and compile our shader program
	// ------------------------------------
	Shader nullShader(FileSystem::getSamplePath("shader/null_technique.vert").c_str(),
		FileSystem::getSamplePath("shader/null_technique.frag").c_str());
	Shader shadowVolume(FileSystem::getSamplePath("shader/shadow_volume.vert").c_str(),
		FileSystem::getSamplePath("shader/shadow_volume.frag").c_str(),
		FileSystem::getSamplePath("shader/shadow_volume.geom").c_str());
	Shader shadowVolumeViz(FileSystem::getSamplePath("shader/shadow_volume.vert").c_str(),
		FileSystem::getSamplePath("shader/const.frag").c_str(),
		FileSystem::getSamplePath("shader/shadow_volumeVis.geom").c_str());
	Shader sceneShader(FileSystem::getSamplePath("shader/shadow_scene.vert").c_str(),
		FileSystem::getSamplePath("shader/shadow_scene.frag").c_str());
	Shader lightShader(FileSystem::getSamplePath("shader/light.vert").c_str(),
		FileSystem::getSamplePath("shader/const.frag").c_str());

	// lighting info
	glm::vec3 lightPos(2.0f, 2.5f, 1.5f);

	// render loop
	// -----------
	while (!glfwWindowShouldClose(window))
	{
		// Set frame time
		float currentFrame = 0;// glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		// Check and call events
		processInput(window);

		// orbiting point light to show moving silhouettes
		lightPos.x = 2.5f * cos(currentFrame * 0.6f);
		lightPos.z = 2.5f * sin(currentFrame * 0.6f);
		lightPos.y = 1.5f + sin(currentFrame * 0.8f) * 0.5f;

		// draw full/wireframe for color passes (volume pass is forced to fill)
		GLenum polygonMode = wireframe ? GL_LINE : GL_FILL;
		glPolygonMode(GL_FRONT_AND_BACK, polygonMode);

		// render
		// ------
		glDepthMask(GL_TRUE);
		glDrawBuffer(GL_BACK);
		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
		glStencilMask(0xFF);
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
		glDrawBuffer(GL_NONE);
		
		// calculate matrices
		glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
		glm::mat4 view = camera.GetViewMatrix();

		glm::mat4 cubeModel(1.0f);
		cubeModel = glm::translate(cubeModel, glm::vec3(0.0f, -0.25f, 0.0f));
		cubeModel = glm::rotate(cubeModel, currentFrame * glm::radians(20.0f), glm::vec3(0.4f, 1.0f, 0.2f));

		glm::mat4 planeModel(1.0f);
		planeModel = glm::translate(planeModel, glm::vec3(0.0f, -1.5f, 0.0f));
		planeModel = glm::scale(planeModel, glm::vec3(8.0f, 1.0f, 8.0f));

		// 0. Depth pre-pass
		// -----------------------------------------------
		// render entire scene into depth buffer, without touching the color buffer
		glDepthMask(GL_TRUE);
		glDisable(GL_STENCIL_TEST);

		nullShader.use();
		nullShader.setMat4("projection", projection);
		nullShader.setMat4("view", view);
		nullShader.setMat4("model", cubeModel);
		renderCube();
		nullShader.setMat4("model", planeModel);
		glDisable(GL_CULL_FACE);
		renderPlane();
		glEnable(GL_CULL_FACE);

		// 1. RenderShadowVolIntoStencil
		// --------------------------------------------
		if (enable_shadows) {
			glEnable(GL_STENCIL_TEST);
			glStencilMask(0xFF);

			// disable writes to the depth buffer (NOTE: writes to color are still disabled from the previous step)
			// only update stencil buffer
			glDepthMask(GL_FALSE);
			// avoid near/far clipping of the volume
			glEnable(GL_DEPTH_CLAMP);
			// disable back face culling, algorithm depends on rendering all the triangles of the volume
			glDisable(GL_CULL_FACE);
			// the volume needs to be rendered solid for correct stencil increments
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

			// We need the stencil test to be enabled but we want it
			// to succeed always. Only the depth test matters.
			glStencilFunc(GL_ALWAYS, 0, 0xff);

			// Set the stencil test per the depth fail algorithm
			glStencilOpSeparate(GL_BACK, GL_KEEP, GL_INCR_WRAP, GL_KEEP);
			glStencilOpSeparate(GL_FRONT, GL_KEEP, GL_DECR_WRAP, GL_KEEP);

			shadowVolume.use();
			glm::mat4 wvp = projection * view * cubeModel;
			shadowVolume.setMat4("gWVP", wvp);
			shadowVolume.setVec3("gLightPos", lightPos);
			renderVolumeCube();

			// Restore local stuff
			glDisable(GL_DEPTH_CLAMP);
			glEnable(GL_CULL_FACE);
		}

		// 2. RenderShadowedScene
		// --------------------------------------------
		glDrawBuffer(GL_BACK);
		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
		// Depth test against the pre-pass and allow tiny fp error to avoid z-flicker; keep depth writes disabled like the reference
		glDepthMask(GL_FALSE);
		glPolygonMode(GL_FRONT_AND_BACK, polygonMode);
		if (enable_shadows) {
			// Draw only if the corresponding stencil value is zero
			glStencilFunc(GL_EQUAL, 0x0, 0xFF);
			glStencilOpSeparate(GL_BACK, GL_KEEP, GL_KEEP, GL_KEEP);
			glStencilOpSeparate(GL_FRONT, GL_KEEP, GL_KEEP, GL_KEEP);
			glStencilMask(0xFF);
			glEnable(GL_STENCIL_TEST);
		}
		else {
			glDisable(GL_STENCIL_TEST);
			glStencilMask(0xFF);
		}

		sceneShader.use();
		sceneShader.setMat4("projection", projection);
		sceneShader.setMat4("view", view);
		sceneShader.setVec3("lightPos", lightPos);
		sceneShader.setVec3("viewPos", camera.Position);
		sceneShader.setInt("ambientOnly", 0);
		sceneShader.setFloat("ambientStrength", enable_shadows ? 0.0f : 0.2f);
		sceneShader.setFloat("specularStrength", 0.5f);
		sceneShader.setFloat("shininess", 32.0f);

		sceneShader.setMat4("model", planeModel);
		sceneShader.setVec3("baseColor", glm::vec3(0.55f, 0.55f, 0.6f));
		glDisable(GL_CULL_FACE);
		renderPlane();
		glEnable(GL_CULL_FACE);

		sceneShader.setMat4("model", cubeModel);
		sceneShader.setVec3("baseColor", glm::vec3(0.85f, 0.3f, 0.2f));
		renderCube();

		// 3. RenderAmbientLight
		// --------------------------------------------
		if (enable_shadows) {
			// enable blending to merge the results of the previous pass with this one
			glEnable(GL_BLEND);
			glBlendEquation(GL_FUNC_ADD);
			glBlendFunc(GL_ONE, GL_ONE);
			glDisable(GL_STENCIL_TEST);
			// keep the ambient pass solid to avoid missing fragments
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			glDepthMask(GL_FALSE);

			sceneShader.setInt("ambientOnly", 1);
			sceneShader.setFloat("ambientStrength", 0.2f);

			sceneShader.setMat4("model", planeModel);
			sceneShader.setVec3("baseColor", glm::vec3(0.55f, 0.55f, 0.6f));
			glDisable(GL_CULL_FACE);
			renderPlane();
			glEnable(GL_CULL_FACE);

			sceneShader.setMat4("model", cubeModel);
			sceneShader.setVec3("baseColor", glm::vec3(0.85f, 0.3f, 0.2f));
			renderCube();

			glDisable(GL_BLEND);
			glPolygonMode(GL_FRONT_AND_BACK, polygonMode);
		}

		// done shading; allow later draws (viz/light) to write depth as usual
		glDepthMask(GL_TRUE);

		// visualize shadow volumes if desired
		if (showVolumes) {
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			shadowVolumeViz.use();
			glm::mat4 wvpViz = projection * view * cubeModel;
			shadowVolumeViz.setMat4("gWVP", wvpViz);
			shadowVolumeViz.setVec3("gLightPos", lightPos);
			renderVolumeCube();
			glPolygonMode(GL_FRONT_AND_BACK, polygonMode);
		}

		// render light source
		if (visLight) {
			lightShader.use();
			lightShader.setMat4("projection", projection);
			lightShader.setMat4("view", view);
			glm::mat4 model(1.0f);
			model = glm::translate(model, lightPos);
			model = glm::scale(model, glm::vec3(0.1f));
			lightShader.setMat4("model", model);
			renderSphere();
		}

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

struct EdgeInfo {
	unsigned int first = std::numeric_limits<unsigned int>::max();
	unsigned int second = std::numeric_limits<unsigned int>::max();
};

std::vector<unsigned int> buildAdjacency(const std::vector<unsigned int>& baseIndices)
{
	const unsigned int INVALID = std::numeric_limits<unsigned int>::max();
	std::unordered_map<EdgeKey, EdgeInfo, EdgeKeyHash> edgeOpposites;

	for (size_t i = 0; i + 2 < baseIndices.size(); i += 3) {
		unsigned int v0 = baseIndices[i];
		unsigned int v1 = baseIndices[i + 1];
		unsigned int v2 = baseIndices[i + 2];

		auto addEdge = [&](unsigned int a, unsigned int b, unsigned int opp) {
			EdgeKey key{ std::min(a, b), std::max(a, b) };
			EdgeInfo& info = edgeOpposites[key];
			if (info.first == INVALID) {
				info.first = opp;
			}
			else if (info.second == INVALID && info.first != opp) {
				info.second = opp;
			}
		};

		addEdge(v0, v1, v2);
		addEdge(v1, v2, v0);
		addEdge(v2, v0, v1);
	}

	std::vector<unsigned int> adjacency;
	adjacency.reserve(baseIndices.size() * 2);

	auto lookup = [&](unsigned int a, unsigned int b, unsigned int selfOpp) {
		EdgeKey key{ std::min(a, b), std::max(a, b) };
		auto it = edgeOpposites.find(key);
		if (it == edgeOpposites.end()) {
			return selfOpp;
		}
		if (it->second.first != INVALID && it->second.first != selfOpp) {
			return it->second.first;
		}
		if (it->second.second != INVALID && it->second.second != selfOpp) {
			return it->second.second;
		}
		return selfOpp;
	};

	for (size_t i = 0; i + 2 < baseIndices.size(); i += 3) {
		unsigned int v0 = baseIndices[i];
		unsigned int v1 = baseIndices[i + 1];
		unsigned int v2 = baseIndices[i + 2];

		adjacency.push_back(v0);
		adjacency.push_back(lookup(v0, v1, v2));
		adjacency.push_back(v1);
		adjacency.push_back(lookup(v1, v2, v0));
		adjacency.push_back(v2);
		adjacency.push_back(lookup(v2, v0, v1));
	}

	return adjacency;
}

// renderPlane() renders a large plane used as a receiver.
// -------------------------------------------------
unsigned int planeVAO = 0;
unsigned int planeVBO = 0;
void renderPlane()
{
	if (planeVAO == 0)
	{
		float planeVertices[] = {
			// positions            // normals
			-1.0f, 0.0f, -1.0f,      0.0f, 1.0f, 0.0f,
			-1.0f, 0.0f,  1.0f,      0.0f, 1.0f, 0.0f,
			 1.0f, 0.0f,  1.0f,      0.0f, 1.0f, 0.0f,
			-1.0f, 0.0f, -1.0f,      0.0f, 1.0f, 0.0f,
			 1.0f, 0.0f,  1.0f,      0.0f, 1.0f, 0.0f,
			 1.0f, 0.0f, -1.0f,      0.0f, 1.0f, 0.0f
		};
		glGenVertexArrays(1, &planeVAO);
		glGenBuffers(1, &planeVBO);
		glBindVertexArray(planeVAO);
		glBindBuffer(GL_ARRAY_BUFFER, planeVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(planeVertices), planeVertices, GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
		glBindVertexArray(0);
	}
	glBindVertexArray(planeVAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindVertexArray(0);
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

// render cube with adjacency info for the shadow volume pass.
// -------------------------------------------------
unsigned int volumeVAO = 0;
unsigned int volumeVBO = 0;
unsigned int volumeEBO = 0;
GLsizei volumeIndexCount = 0;
void renderVolumeCube()
{
	if (volumeVAO == 0)
	{
		float vertices[] = {
			-1.0f, -1.0f, -1.0f, // 0
			 1.0f, -1.0f, -1.0f, // 1
			 1.0f,  1.0f, -1.0f, // 2
			-1.0f,  1.0f, -1.0f, // 3
			-1.0f, -1.0f,  1.0f, // 4
			 1.0f, -1.0f,  1.0f, // 5
			 1.0f,  1.0f,  1.0f, // 6
			-1.0f,  1.0f,  1.0f  // 7
		};

		// 12 triangles (two per face) CCW winding outward
		std::vector<unsigned int> baseIndices = {
			// back (-Z)
			0, 2, 1,
			2, 0, 3,
			// front (+Z)
			4, 5, 6,
			6, 7, 4,
			// left (-X)
			7, 3, 0,
			0, 4, 7,
			// right (+X)
			6, 1, 2,
			1, 6, 5,
			// bottom (-Y)
			0, 1, 5,
			5, 4, 0,
			// top (+Y)
			3, 6, 2,
			6, 3, 7
		};

		std::vector<unsigned int> adjacency = buildAdjacency(baseIndices);
		volumeIndexCount = static_cast<GLsizei>(adjacency.size());

		glGenVertexArrays(1, &volumeVAO);
		glGenBuffers(1, &volumeVBO);
		glGenBuffers(1, &volumeEBO);

		// link vertex attributes
		glBindVertexArray(volumeVAO);

		// fill buffer
		glBindBuffer(GL_ARRAY_BUFFER, volumeVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, volumeEBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, adjacency.size() * sizeof(unsigned int), adjacency.data(), GL_STATIC_DRAW);
		
		glEnableVertexAttribArray(0);
		// positions
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);
	}
	// render Cube with adjacency
	glBindVertexArray(volumeVAO);
	glDrawElements(GL_TRIANGLES_ADJACENCY, volumeIndexCount, GL_UNSIGNED_INT, 0);
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

	if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS && !space_pressed)
	{
		enable_shadows = !enable_shadows;
		space_pressed = true;
	}
	if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_RELEASE)
	{
		space_pressed = false;
	}

	if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS && !l_pressed)
	{
		visLight = !visLight;
		l_pressed = true;
	}
	if (glfwGetKey(window, GLFW_KEY_L) == GLFW_RELEASE)
	{
		l_pressed = false;
	}

	if (glfwGetKey(window, GLFW_KEY_V) == GLFW_PRESS && !v_pressed)
	{
		showVolumes = !showVolumes;
		v_pressed = true;
	}
	if (glfwGetKey(window, GLFW_KEY_V) == GLFW_RELEASE)
	{
		v_pressed = false;
	}

	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
		w_pressed = true;
	}
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_RELEASE && w_pressed) {
		wireframe = !wireframe;
		w_pressed = false;
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
			lastX = static_cast<float>(xpos);
			lastY = static_cast<float>(ypos);
			firstMouse = false;
		}

		float xoffset = static_cast<float>(xpos - lastX);
		float yoffset = static_cast<float>(lastY - ypos);
		// reversed since y-coordinates range from bottom to top
		lastX = static_cast<float>(xpos);
		lastY = static_cast<float>(ypos);

		camera.rotate(xoffset, yoffset);
	} 

	//Moving 
	if (glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS &&
		glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS) {
		if (firstMouse)
		{
			lastX = static_cast<float>(xpos);
			lastY = static_cast<float>(ypos);
			firstMouse = false;
		}

		float xoffset = static_cast<float>(xpos - lastX);
		float yoffset = static_cast<float>(lastY - ypos);
		// reversed since y-coordinates range from bottom to top
		lastX = static_cast<float>(xpos);
		lastY = static_cast<float>(ypos);

		if (xoffset > 0) {
			camera.translate(LEFT, xoffset*0.05f);
		}
		else {
			camera.translate(RIGHT, xoffset*-0.05f);
		}

		if (yoffset > 0) {
			camera.translate(DOWN, yoffset*0.05f);
		}
		else {
			camera.translate(UP, yoffset*-0.05f);
		}
	}
	
	//Zoom 
	if (glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS &&
		glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
		if (firstMouse)
		{
			lastX = static_cast<float>(xpos);
			firstMouse = false;
		}

		float xoffset = static_cast<float>(xpos - lastX);
		lastX = static_cast<float>(xpos);

		if (xoffset > 0) {
			camera.translate(FORWARD, xoffset*0.05f);
		}
		else {
			camera.translate(BACKWARD, xoffset*-0.05f);
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
	camera.ProcessMouseScroll(static_cast<float>(yoffset));
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	// make sure the viewport matches the new window dimensions; note that width and 
	// height will be significantly larger than specified on retina displays.
	glViewport(0, 0, width, height);
}

// renders (and builds at first invocation) a sphere
// -------------------------------------------------
unsigned int sphereVAO = 0;
unsigned int indexCount;
void renderSphere()
{
	if (sphereVAO == 0)
	{
		glGenVertexArrays(1, &sphereVAO);

		unsigned int vbo, ebo;
		glGenBuffers(1, &vbo);
		glGenBuffers(1, &ebo);

		std::vector<glm::vec3> positions;
		std::vector<glm::vec2> uv;
		std::vector<glm::vec3> normals;
		std::vector<unsigned int> indices;

		const unsigned int X_SEGMENTS = 36;
		const unsigned int Y_SEGMENTS = 36;
		const float PI = 3.14159265359f;
		for (unsigned int y = 0; y <= Y_SEGMENTS; ++y)
		{
			for (unsigned int x = 0; x <= X_SEGMENTS; ++x)
			{
				float xSegment = static_cast<float>(x) / static_cast<float>(X_SEGMENTS);
				float ySegment = static_cast<float>(y) / static_cast<float>(Y_SEGMENTS);
				float xPos = std::cos(xSegment * 2.0f * PI) * std::sin(ySegment * PI);
				float yPos = std::cos(ySegment * PI);
				float zPos = std::sin(xSegment * 2.0f * PI) * std::sin(ySegment * PI);

				positions.push_back(glm::vec3(xPos, yPos, zPos));
				uv.push_back(glm::vec2(xSegment, ySegment));
				normals.push_back(glm::vec3(xPos, yPos, zPos));
			}
		}

		bool oddRow = false;
		for (unsigned int y = 0; y < Y_SEGMENTS; ++y)
		{
			if (!oddRow) // even rows: y == 0, y == 2; and so on
			{
				for (unsigned int x = 0; x <= X_SEGMENTS; ++x)
				{
					indices.push_back(y * (X_SEGMENTS + 1) + x);
					indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
				}
			}
			else
			{
				for (int x = X_SEGMENTS; x >= 0; --x)
				{
					indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
					indices.push_back(y * (X_SEGMENTS + 1) + x);
				}
			}
			oddRow = !oddRow;
		}
		indexCount = static_cast<unsigned int>(indices.size());

		std::vector<float> data;
		for (size_t i = 0; i < positions.size(); ++i)
		{
			data.push_back(positions[i].x);
			data.push_back(positions[i].y);
			data.push_back(positions[i].z);
			if (!uv.empty())
			{
				data.push_back(uv[i].x);
				data.push_back(uv[i].y);
			}
			if (!normals.empty())
			{
				data.push_back(normals[i].x);
				data.push_back(normals[i].y);
				data.push_back(normals[i].z);
			}
		}
		glBindVertexArray(sphereVAO);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(float), data.data(), GL_STATIC_DRAW);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
		GLsizei stride = static_cast<GLsizei>((3 + 2 + 3) * sizeof(float));
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, stride, (void*)(5 * sizeof(float)));
	}

	glBindVertexArray(sphereVAO);
	glDrawElements(GL_TRIANGLE_STRIP, indexCount, GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);
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
