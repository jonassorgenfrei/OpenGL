/* 
 *	Stencil Shadow Volume Technique
 *  when an object is inside the volume (in shadow) the front polygons of the volume win the depth test
 *  against the polygons of the object and the back polygons of the volume fail the same test
 * 
 * NOTE: THIS PROGRAM IS WORK IN PROGESS AND NOT WORKING YET!
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

#include "light.h"
#include "material.h"
#include "filesystem.h"

#include <iostream>

// callbacks
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
// current Mouse position 
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow *window);
/* load texture */
unsigned int loadTexture(const char *path, bool gammaCorrection);
void renderScene(const Shader &shader);
void renderCube();
void renderSphere();

void icon(GLFWwindow* window);

// settings
const unsigned int SCR_WIDTH = 1280;
const unsigned int SCR_HEIGHT = 1024;

bool w_pressed = false;
bool l_pressed = false;

bool visLight = false;
bool shadows = false;

bool space_pressed = false;
bool wireframe = false;

// camera
Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
float lastX = SCR_WIDTH / 2.0;
float lastY = SCR_HEIGHT / 2.0;
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
		glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // uncomment this statement to fix compilation on OS X
	#endif

	// glfw window creation
	// --------------------
	GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Advanced GL", NULL, NULL);

	/* creates and configures default framebuffer */
	// NOTE: glfw creates a stencil buffer automatically
	// other libs may not
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
	/* Face Culling */
	glEnable(GL_CULL_FACE);
	// to avoid z fighting from pixels
	// relaxes the depth test a bit, if a second pixel is rendered on top of a previous pixel with the same
	// depth the last pixel always take precedence
	glDepthFunc(GL_LEQUAL);

	// build and compile our shader program
	// ------------------------------------
	Shader nullShader(FileSystem::getPath("shader/null_technique.vert").c_str(), FileSystem::getPath("shader/null_technique.frag").c_str());
	Shader shadowVolume(FileSystem::getPath("shader/shadow_volume.vert").c_str(), FileSystem::getPath("shader/shadow_volume.frag").c_str(), FileSystem::getPath("shader/shadow_volume.geom").c_str());
	//Shader lightShader(FileSystem::getPath("shader/light.vert").c_str(), FileSystem::getPath("shader/light.frag").c_str());
	Shader shadowVolumeViz(FileSystem::getPath("shader/shadow_volume.vert").c_str(), FileSystem::getPath("shader/const.frag").c_str(), FileSystem::getPath("shader/shadow_volumeVis.geom").c_str());
	//Shader shadowVolumeViz(FileSystem::getPath("shader/shadow_volume.vert").c_str(), FileSystem::getPath("shader/const.frag").c_str());

	/*
	LGL CODE: 
	// load textures
	// -------------
	unsigned int woodTexture = loadTexture(FileSystem::getPath("../../content/images/wood.png").c_str(), false);

	// configure depth map FBO
	// -----------------------
	const unsigned int SHADOW_WIDTH = 1024, SHADOW_HEIGHT = 1024; // Size of Cubemap images
	unsigned int depthMapFBO;
	glGenFramebuffers(1, &depthMapFBO);

	// create a depth cubemap texture 
	unsigned int depthCubemap;	
	glGenTextures(1, &depthCubemap);

	// Create each of the single cubemap faces as 2D depth-valued texture images
	glBindTexture(GL_TEXTURE_CUBE_MAP, depthCubemap);
	for (unsigned int i = 0; i < 6; ++i) {
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X+i, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	}
	// set texture parameters
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	// attach (all) depth textures as FBO's depth buffer, since we are going to use a geometry shader 
	// that allows to render to all faces in a single pass
	glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthCubemap, 0);
	glDrawBuffer(GL_NONE); // explicitly tell OpenGL this framebuffer object does not
	glReadBuffer(GL_NONE); // render to a color buffer
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// shader configuration
	  // --------------------
	shader.use();
	shader.setInt("diffuseTexture", 0);
	shader.setInt("depthMap", 1);

	
	*/

	// lighting info
	// -------------
	glm::vec3 lightPos(0.0f, 0.0f, 0.0f);

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

		// move light position over time
		lightPos.z = sin(glfwGetTime() * 0.5) * 3.0;

		//m_scale += 0.1f;

		//m_pGameCamera->OnRender();

		// draw full/wireframe
		glPolygonMode(GL_FRONT_AND_BACK, wireframe ? GL_LINE : GL_FILL);

		glDepthMask(GL_TRUE);

		// render
		// ------
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
		
		// set uniforms
		glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
		glm::mat4 view = camera.GetViewMatrix();
		glm::mat4 model = glm::mat4(1.0f);

		// 0. RenderSceneIntoDepth
		// -----------------------------------------------
		// render entire scene into depth buffer, without touching the color buffer
		glDrawBuffer(GL_NONE);	// disable writes to the color buffer
		// if the depth buffer is only partially updated we will get incorrect results
		nullShader.use();
		nullShader.setMat4("projection", projection);
		nullShader.setMat4("view", view);
		nullShader.setMat4("model", model);

		/*
			Pipeline p;

			p.SetCamera(m_pGameCamera->GetPos(), m_pGameCamera->GetTarget(), m_pGameCamera->GetUp());
			p.SetPerspectiveProj(m_persProjInfo);

			m_boxOrientation.m_rotation = Vector3f(0, m_scale, 0);
			p.Orient(m_boxOrientation);
			m_nullTech.SetWVP(p.GetWVPTrans());
			m_box.Render();

			p.Orient(m_quadOrientation);
			m_nullTech.SetWVP(p.GetWVPTrans());
			m_quad.Render();
		*/

		// 1. RenderShadowVolIntoStencil
		// --------------------------------------------
		// render shadow volume into the stencil buffer while setting up the stencil test
		// it generates the volume (and its caps) from the silhouette of the occluder
		glEnable(GL_STENCIL_TEST);

		// disable writes to the depth buffer (NOTE: writes to color are still disabled from the previous step)
		// only update stencil buffer
		glDepthMask(GL_FALSE);
		// enable depth clamp, cause projected-to-infinity-vertices (from the far cap) to be clamped to
		// the maximum depth value (otherwise the far cap would be clipped away
		glEnable(GL_DEPTH_CLAMP);
		// disable back face culling, algorithms depends on rendering all the triangles of the volume
		glDisable(GL_CULL_FACE);

		// We need the stencil test to be enabled but we want it
		// to succeed always. Only the depth test matters.
		glStencilFunc(GL_ALWAYS, 0, 0xff);

		// Set the stencil test per the depth fail algorithm
		glStencilOpSeparate(GL_BACK, GL_KEEP, GL_INCR_WRAP, GL_KEEP);
		glStencilOpSeparate(GL_FRONT, GL_KEEP, GL_DECR_WRAP, GL_KEEP);

		shadowVolume.use();

		/*
			m_ShadowVolTech.SetLightPos(m_pointLight.Position);

			// Render the occluder
			Pipeline p;
			p.SetCamera(m_pGameCamera->GetPos(), m_pGameCamera->GetTarget(), m_pGameCamera->GetUp());
			p.SetPerspectiveProj(m_persProjInfo);
			m_boxOrientation.m_rotation = Vector3f(0, m_scale, 0);
			p.Orient(m_boxOrientation);
			m_ShadowVolTech.SetVP(p.GetVPTrans());
			m_ShadowVolTech.SetWorldMatrix(p.GetWorldTrans());
			m_box.Render();
		*/

		// Restore local stuff
		glDisable(GL_DEPTH_CLAMP);
		glEnable(GL_CULL_FACE);

		// 2. RenderShadowedScene
		// --------------------------------------------
		// render the scene while taking the values in the stencil buffer into account
		// only render pixels whose stencil value is zero
		// enable writing in color buffer
		glDrawBuffer(GL_BACK);

		// Draw only if the corresponding stencil value is zero
		glStencilFunc(GL_EQUAL, 0x0, 0xFF);

		// prevent update to the stencil buffer
		glStencilOpSeparate(GL_BACK, GL_KEEP, GL_KEEP, GL_KEEP);

		/*
			m_LightingTech.Enable();

			m_pointLight.AmbientIntensity = 0.0f;
			m_pointLight.DiffuseIntensity = 0.8f;

			m_LightingTech.SetPointLights(1, &m_pointLight);

			Pipeline p;
			p.SetPerspectiveProj(m_persProjInfo);
			p.SetCamera(m_pGameCamera->GetPos(), m_pGameCamera->GetTarget(), m_pGameCamera->GetUp());

			m_boxOrientation.m_rotation = Vector3f(0, m_scale, 0);
			p.Orient(m_boxOrientation);
			m_LightingTech.SetWVP(p.GetWVPTrans());
			m_LightingTech.SetWorldMatrix(p.GetWorldTrans());
			m_box.Render();

			p.Orient(m_quadOrientation);
			m_LightingTech.SetWVP(p.GetWVPTrans());
			m_LightingTech.SetWorldMatrix(p.GetWorldTrans());
			m_pGroundTex->Bind(COLOR_TEXTURE_UNIT);
			m_quad.Render();
		*/
		glDisable(GL_STENCIL_TEST);

		// 3. RenderAmbientLight
		// --------------------------------------------
		// add extra ambient render pass to avoid black pixels, that were dropped by the stencil test
		
		// enable blending to merge the resutls of the previous pass with this one
		glEnable(GL_BLEND);
		glBlendEquation(GL_FUNC_ADD);
		glBlendFunc(GL_ONE, GL_ONE);
		/*
		nullShader.setMat4("projection", projection);
		nullShader.setMat4("view", view);
		// room cube
		glm::mat4 model = glm::mat4(1.0);
		model = glm::scale(model, glm::vec3(5.0f));
		shader.setMat4("model", model);
		glCullFace(GL_FRONT);
		//glDisable(GL_CULL_FACE); // note that we disable culling here since we render 'inside' the cube instead of the usual 'outside' which throws off the normal culling methods.
		shader.setInt("reverse_normals", 1); // A small little hack to invert normals when drawing cube from the inside so lighting still works.
		renderCube();
		shader.setInt("reverse_normals", 0); // and of course disable it
		glCullFace(GL_BACK);
		//glEnable(GL_CULL_FACE);
		// cubes
		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(4.0f, -3.5f, 0.0));
		model = glm::scale(model, glm::vec3(0.5f));
		shader.setMat4("model", model);
		renderCube();
		*/
		/*
		m_LightingTech.Enable();

		m_pointLight.AmbientIntensity = 0.2f;
		m_pointLight.DiffuseIntensity = 0.0f;	// note: zero out diffuse intensity

		m_LightingTech.SetPointLights(1, &m_pointLight);

		m_pGroundTex->Bind(GL_TEXTURE0);

		Pipeline p;
		p.SetPerspectiveProj(m_persProjInfo);
		p.SetCamera(m_pGameCamera->GetPos(), m_pGameCamera->GetTarget(), m_pGameCamera->GetUp());

		m_boxOrientation.m_rotation = Vector3f(0, m_scale, 0);
		p.Orient(m_boxOrientation);
		m_LightingTech.SetWVP(p.GetWVPTrans());
		m_LightingTech.SetWorldMatrix(p.GetWorldTrans());
		m_box.Render();

		p.Orient(m_quadOrientation);
		m_LightingTech.SetWVP(p.GetWVPTrans());
		m_LightingTech.SetWorldMatrix(p.GetWorldTrans());
		m_pGroundTex->Bind(COLOR_TEXTURE_UNIT);
		m_quad.Render();
		*/
		glDisable(GL_BLEND);
		//glDisable(GL_CULL_FACE);
		// visualize shadow volumes
		shadowVolumeViz.use();
		shadowVolumeViz.setMat4("projection", projection);
		shadowVolumeViz.setMat4("view", view);

		// cubes
		model = glm::mat4(1.0f);
		//model = glm::translate(model, glm::vec3(4.0f, -3.5f, 0.0));
		model = glm::scale(model, glm::vec3(0.5f));
		shadowVolumeViz.setMat4("model", model);
		shadowVolumeViz.setVec3("lightPos", lightPos);
		renderCube();


		/*
		LGL CODE: 
		float aspect = (float)SHADOW_WIDTH / (float)SHADOW_HEIGHT;
		float nearP = 1.0f;
		float far_plane = 25.0f;
		glm::mat4 shadowProj = glm::perspective(glm::radians(90.0f), aspect, nearP, far_plane); // reusable for each trans. matrix
		// field of view -> 90 degrees, viewing field is exactly large enough to properly fill a
		// single face of the cubemap --> all faces align correctly to each other at the edges

		// we need 6 different view-matrices (per direction)
		// 6 different light space transformation matrices
		std::vector<glm::mat4> shadowTransforms;
		shadowTransforms.push_back(shadowProj *
			glm::lookAt(lightPos, lightPos + glm::vec3(1.0, 0.0, 0.0), glm::vec3(0.0, -1.0, 0.0))); // right
		shadowTransforms.push_back(shadowProj *
			glm::lookAt(lightPos, lightPos + glm::vec3(-1.0, 0.0, 0.0), glm::vec3(0.0, -1.0, 0.0))); // left
		shadowTransforms.push_back(shadowProj *
			glm::lookAt(lightPos, lightPos + glm::vec3(0.0, 1.0, 0.0), glm::vec3(0.0, 0.0, 1.0))); // top
		shadowTransforms.push_back(shadowProj *
			glm::lookAt(lightPos, lightPos + glm::vec3(0.0, -1.0, 0.0), glm::vec3(0.0, 0.0, -1.0))); // bottom
		shadowTransforms.push_back(shadowProj *
			glm::lookAt(lightPos, lightPos + glm::vec3(0.0, 0.0, 1.0), glm::vec3(0.0, -1.0, 0.0))); // near
		shadowTransforms.push_back(shadowProj *
			glm::lookAt(lightPos, lightPos + glm::vec3(0.0, 0.0, -1.0), glm::vec3(0.0, -1.0, 0.0))); // far		

		
		glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
		glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
			glClear(GL_DEPTH_BUFFER_BIT);
			simpleDepthShader.use();
			for (unsigned int i = 0; i < 6; ++i)
				simpleDepthShader.setMat4("shadowMatrices[" + std::to_string(i) + "]", shadowTransforms[i]);
			simpleDepthShader.setFloat("far_plane", far_plane);
			simpleDepthShader.setVec3("lightPos", lightPos);
			renderScene(simpleDepthShader);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		// 2. render scene as normal
		// -------------------------
		glViewport(0,0, SCR_WIDTH, SCR_HEIGHT);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		shader.use();

		// set uniforms
		glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
		glm::mat4 view = camera.GetViewMatrix();
		shader.setMat4("projection", projection);
		shader.setMat4("view", view);
		// set lighting uniforms
		shader.setVec3("lightPos", lightPos);
		shader.setVec3("viewPos", camera.Position);
		shader.setInt("shadows", shadows); // enable/disable shadows by pressing 'SPACE'
		shader.setFloat("far_plane", far_plane);
		
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, woodTexture);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_CUBE_MAP, depthCubemap);
		renderScene(shader); // render some cubes in a large cube room scattered around a light source at the center of the scene

		if (visLight) {
			// render light source
			lightShader.use();
			lightShader.setMat4("projection", projection);
			lightShader.setMat4("view", view);
			glm::mat4 model = glm::mat4(1.0);
			model = glm::translate(model, lightPos);
			model = glm::scale(model, glm::vec3(0.1f));
			lightShader.setMat4("model", model);
			renderSphere();
		}
		*/
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

// renders the 3D scene
// --------------------
void renderScene(const Shader &shader)
{
	// room cube
	glm::mat4 model = glm::mat4(1.0);
	model = glm::scale(model, glm::vec3(5.0f));
	shader.setMat4("model", model);
	glCullFace(GL_FRONT);
	//glDisable(GL_CULL_FACE); // note that we disable culling here since we render 'inside' the cube instead of the usual 'outside' which throws off the normal culling methods.
	shader.setInt("reverse_normals", 1); // A small little hack to invert normals when drawing cube from the inside so lighting still works.
	renderCube();
	shader.setInt("reverse_normals", 0); // and of course disable it
	glCullFace(GL_BACK);
	//glEnable(GL_CULL_FACE);
	// cubes
	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(4.0f, -3.5f, 0.0));
	model = glm::scale(model, glm::vec3(0.5f));
	shader.setMat4("model", model);
	renderCube();
	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(2.0f, 3.0f, 1.0));
	model = glm::scale(model, glm::vec3(0.75f));
	shader.setMat4("model", model);
	renderCube();
	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(-3.0f, -1.0f, 0.0));
	model = glm::scale(model, glm::vec3(0.5f));
	shader.setMat4("model", model);
	renderCube();
	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(-1.5f, 1.0f, 1.5));
	model = glm::scale(model, glm::vec3(0.5f));
	shader.setMat4("model", model);
	renderCube();
	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(-1.5f, 2.0f, -3.0));
	model = glm::rotate(model, glm::radians(60.0f), glm::normalize(glm::vec3(1.0, 0.0, 1.0)));
	model = glm::scale(model, glm::vec3(0.75f));
	shader.setMat4("model", model);
	renderCube();
}


// renderCube() renders a 1x1 3D cube in NDC.
// -------------------------------------------------
unsigned int cubeVAO = 0;
unsigned int cubeVBO = 0;
unsigned int cubeEBO = 0;
void renderCube()
{
	// initialize (if necessary)
	if (cubeVAO == 0)
	{
		// TODO: FIX ADJACENCY RENDERING
		// Points 0, 2, and 4 define the triangle.
		// Points 1, 3, and 5 tell where adjacent triangles are

		// Vertex array (assuming each vertex has 3 coordinates)
		
		GLfloat vertices[] = {
			// Vertex 0
			 1.0f, -1.0f,  1.0f,
			// Vertex 1
			-1.0f, -1.0f,  1.0f,
			// Vertex 2
			 1.0f,  1.0f,  1.0f,
			// Vertex 3
			-1.0f,  1.0f,  1.0f,
			// Vertex 4
			-1.0f, -1.0f, -1.0f,
			// Vertex 5
			 1.0f, -1.0f, -1.0f,
			// Vertex 6
			-1.0f,  1.0f, -1.0f,
			// Vertex 7
			 1.0f,  1.0f, -1.0f,
		};

		// Index array
		GLuint indices[] = {
			// Bottom
			0, 3, 1, 4, 5, 2,			
			1, 6, 4, 7, 5, 0,

			// Side 1
			0, 1, 5, 7, 2, 3,
			5, 4, 7, 6, 2, 0,

			// Side 2
			5, 1, 4, 6, 7, 2,
			4, 1, 6, 2, 7, 5,

			// Side 3
			4, 5, 1, 3, 6, 7,
			1, 0, 3, 2, 6, 4,

			// Side 4
			1, 5, 0, 2, 3, 6,
			0, 5, 2, 6, 3, 1,
			// Top
			3, 0, 2, 7, 6, 1,
			2, 5, 7, 4, 6, 3

		};

	
		glGenVertexArrays(1, &cubeVAO);
		glGenBuffers(1, &cubeVBO);
		glGenBuffers(1, &cubeEBO);

		// link vertex attributes
		glBindVertexArray(cubeVAO);

		// fill buffer
		glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cubeEBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
		
		glEnableVertexAttribArray(0);
		// positions
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
		// normals
		//glEnableVertexAttribArray(1);
		//glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
		// uv coords
		//glEnableVertexAttribArray(2);
		//glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);
	}
	// render Cube
	glBindVertexArray(cubeVAO);
	// This function takes the topology, the number of indicesand their type.
	// The fourth parameter tells it where to start in the index buffer.
	glDrawElementsBaseVertex(GL_TRIANGLES_ADJACENCY, 6*2*(3+3), GL_UNSIGNED_INT, 0, 0);
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

	if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS && !space_pressed)
	{
		shadows = !shadows;
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
		const float PI = 3.14159265359;
		for (unsigned int y = 0; y <= Y_SEGMENTS; ++y)
		{
			for (unsigned int x = 0; x <= X_SEGMENTS; ++x)
			{
				float xSegment = (float)x / (float)X_SEGMENTS;
				float ySegment = (float)y / (float)Y_SEGMENTS;
				float xPos = std::cos(xSegment * 2.0f * PI) * std::sin(ySegment * PI);
				float yPos = std::cos(ySegment * PI);
				float zPos = std::sin(xSegment * 2.0f * PI) * std::sin(ySegment * PI);

				positions.push_back(glm::vec3(xPos, yPos, zPos));
				uv.push_back(glm::vec2(xSegment, ySegment));
				normals.push_back(glm::vec3(xPos, yPos, zPos));
			}
		}

		bool oddRow = false;
		for (int y = 0; y < Y_SEGMENTS; ++y)
		{
			if (!oddRow) // even rows: y == 0, y == 2; and so on
			{
				for (int x = 0; x <= X_SEGMENTS; ++x)
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
		indexCount = indices.size();

		std::vector<float> data;
		for (int i = 0; i < positions.size(); ++i)
		{
			data.push_back(positions[i].x);
			data.push_back(positions[i].y);
			data.push_back(positions[i].z);
			if (uv.size() > 0)
			{
				data.push_back(uv[i].x);
				data.push_back(uv[i].y);
			}
			if (normals.size() > 0)
			{
				data.push_back(normals[i].x);
				data.push_back(normals[i].y);
				data.push_back(normals[i].z);
			}
		}
		glBindVertexArray(sphereVAO);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(float), &data[0], GL_STATIC_DRAW);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);
		float stride = (3 + 2 + 3) * sizeof(float);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, stride, (void*)(5 * sizeof(float)));
	}

	glBindVertexArray(sphereVAO);
	glDrawElements(GL_TRIANGLE_STRIP, indexCount, GL_UNSIGNED_INT, 0);
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