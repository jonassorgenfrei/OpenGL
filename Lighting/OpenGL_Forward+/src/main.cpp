/* 
 * Forward+
 */

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#define GLM_FORCE_SILENT_WARNINGS 1

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "shader_m.h"
#include "camera.h"
#include "model.h"
#include "stb_image.h"
#include "gBuffer.h"
#include "filesystem.h"

#include <iostream>

#define randFloat() (float)rand() / (float)RAND_MAX

// callbacks
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
// current Mouse position 
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow *window);

/* render geometry */
void renderSphere();

// helper functions
void repareTilesMemory(int width, int height);

void icon(GLFWwindow* window);

// settings
const unsigned int SCR_WIDTH = 1920;
const unsigned int SCR_HEIGHT = 1080;

// allocate enough storage for debug buffer
const unsigned int DEBUG_POSITIONS_BUFFER_SIZE = 5000000;
const unsigned int TILE_SIZE = 16;

const unsigned int NR_LIGHTS = 100;

GLuint visibleLightsBuffer = NULL;

int framebufferWidth = SCR_WIDTH;
int framebufferHeight = SCR_HEIGHT;

// camera
Camera camera(glm::vec3(-6.8981f, 0.965631f, -0.312417f), glm::vec3(0.0f,1.0f,0.0f), -2.95f, -3.55f);

double lastX = SCR_WIDTH / 2.0;
double lastY = SCR_HEIGHT / 2.0;

bool firstMouse = true;

bool h_pressed = false;
bool w_pressed = false;
bool m_pressed = false;
bool t_pressed = false;
bool l_pressed = false;
bool u_pressed = false;

bool wireframe = false;		// render wireframe flag
bool heatmapVis = false;	// render heatmap over the scene
bool tileVis = false;		// render tiles as grid (renderGrid)
bool lightVis = false;		// render light sources as spheres
bool updateGrid = true;		// if tile-grid has to be recalculated per frame


// timing 
float deltaTime = 0.0f; // time between current frame and last frame
float lastFrame = 0.0f; // time of last frame

struct PointLight {
	// Memorylayout:
	//
	// position    radius
	// |           |   color
	// |           |   |
	// v           v   v
	// +---+---+---+---+---+---+---+---+
	// | x | y | z | R | r | g | b | a |
	// +---+---+---+---+---+---+---+---+
	// 0   4   8   12  16  20  24  28  32 Byte
	glm::vec3 position;
	// this would need padding to align "color" correct.
	// adding radius as float parm to fill the 16 bytes
	// Vec4 has an alginmtn of 4*N, where N is the count of bytes (basic machine units)
	// per component. Even if "position" fills only the bytes 0 to 11,
	// "color" would begin at byte 16.
	// The bytes 12 to 15 would be unused. 
	// The C-Compiler would not add this gap and tho the Memorylayout would be offseted.
	// To avoid this problem radius will be added there.
	// This way both the C-Compiler and the GLSL compiler would create the same memory layout.
	float radius;
	glm::vec4 color;
};


int main()
{
	// glfw: initialize and configure
	// ------------------------------
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	// Resizable Window 
	//glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

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
	
	// GLFW to capture mouse
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

	// glad: load all OpenGL function pointers
	// ---------------------------------------
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
		return -1;
	}

	std::cout << "OpenGL-Version: " << glGetString(GL_VERSION) << std::endl;

	icon(window);

	// configure global opengl state
	// -----------------------------

	// Disable V-Sync 
	// don't wait on vsync, render as fast as possible
	glfwSwapInterval(0);

	/* DEPTH BUFFER */
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);		// Back-Face Culling
	glCullFace(GL_BACK);
	glFrontFace(GL_CCW);

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClearDepth(1);

	// build and compile shader program(s)
	// ------------------------------------
	Shader mainProgram(FileSystem::getPath("shader/main.vert").c_str(), FileSystem::getPath("shader/main.frag").c_str());
	Shader colorProgram(FileSystem::getPath("shader/color.vert").c_str(), FileSystem::getPath("shader/color.frag").c_str());
	Shader uniformColorProgram(FileSystem::getPath("shader/uniformcolor.vert").c_str(), FileSystem::getPath("shader/uniformcolor.frag").c_str());
	Shader tileProgram(FileSystem::getPath("shader/tile.comp").c_str());

	// load models
	// -----------
	std::cout << "Loading Model" << std::endl;
	Model object(FileSystem::getPath("../../content/models/sponza_crytek/sponza.obj").c_str());
	std::cout << "Finished Model Loading" << std::endl;

	// set up buffers
	// --------------

	// debug buffer
	GLuint debugPositionsVertexArray;
	GLuint debugPositions, debugColors;

	glGenVertexArrays(1, &debugPositionsVertexArray);
	glBindVertexArray(debugPositionsVertexArray);

	// create buffer to store positions
	glGenBuffers(1, &debugPositions);
	glBindBuffer(GL_ARRAY_BUFFER, debugPositions);
	glBufferData(GL_ARRAY_BUFFER, DEBUG_POSITIONS_BUFFER_SIZE * sizeof(glm::vec4), NULL, GL_DYNAMIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, NULL);

	// create buffer to store colors
	glGenBuffers(1, &debugColors);
	glBindBuffer(GL_ARRAY_BUFFER, debugColors);
	glBufferData(GL_ARRAY_BUFFER, DEBUG_POSITIONS_BUFFER_SIZE * sizeof(glm::vec4), NULL, GL_DYNAMIC_DRAW);

	glEnableVertexAttribArray(3);
	glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, 0, NULL);

	glBindVertexArray(0);

	// set index for shader storage buffer target
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, debugPositions);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, debugColors);
	
	// lighting info
	// -------------
	std::vector<PointLight> lights;
	for (unsigned int i = 0; i < NR_LIGHTS; i++)
	{
		// calculate slightly random offsets
		PointLight light;
		light.position = glm::vec3(5 * (2 * randFloat() - 1), 5 * randFloat(), 3 * (2 * randFloat() - 1));
		light.radius = 1.5f + 1.5f * randFloat();
		light.color = glm::vec4(randFloat(), randFloat(), randFloat(), 1.0f);
		
		lights.push_back(light);
	}

	// set up buffer to store lights
	GLuint lightsBuffer;
	glGenBuffers(1, &lightsBuffer);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, lightsBuffer);
	glBufferData(GL_SHADER_STORAGE_BUFFER, NR_LIGHTS * sizeof(PointLight), lights.data(), GL_STATIC_DRAW);

	repareTilesMemory(SCR_WIDTH, SCR_HEIGHT);

	// shader configuration
	// --------------------

	// Timer Queries
	GLuint timerQuery[2];
	glGenQueries(2, timerQuery);

	// write to timer query timerQuery[0] and read from timerQuery[1] in first frame
	// get dummy time for timerQuery[1]
	glBeginQuery(GL_TIME_ELAPSED, timerQuery[1]);
	glEndQuery(GL_TIME_ELAPSED);
	
	static int frameIndex = 0;

	// render loop
	// -----------
	while (!glfwWindowShouldClose(window))
	{
		// Set frame time
		auto currentFrame = static_cast<float>(glfwGetTime());
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		// Check and call events
		processInput(window);

		// begin timer
		glBeginQuery(GL_TIME_ELAPSED, timerQuery[frameIndex % 2]);

		// clear Framebuffer
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		// select either wireframe or shaded
		glPolygonMode(GL_FRONT_AND_BACK, wireframe ? GL_LINE : GL_FILL);

		// calulate amount of tiles based on framebuffer size
		int numTilesX = (framebufferWidth + (TILE_SIZE - 1)) / TILE_SIZE;
		int numTilesY = (framebufferHeight + (TILE_SIZE - 1)) / TILE_SIZE;

		// calc projectiong & view matrix
		glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.001f, 100.0f);
		glm::mat4 view = camera.GetViewMatrix();
		// identity model matrix
		glm::mat4 identityMatrix(1.0);

		// ----------------------------------------------------------------------------
		// Light Culling
		// ----------------------------------------------------------------------------

		glm::mat4 invViewProjMat = glm::inverse(projection * view);
		tileProgram.use();

		// set shader parms
		tileProgram.setMat4("InverseViewProjectionMatrix", invViewProjMat);
		tileProgram.setMat4("ViewMatrix", view);
		tileProgram.setInt2("FramebufferSize", framebufferWidth, framebufferHeight);
		tileProgram.setInt2("TileSize", TILE_SIZE, TILE_SIZE);
		tileProgram.setBool("UpdateGrid", updateGrid);

		// run compute shader
		glDispatchCompute(numTilesX, numTilesY, 1);

		// ----------------------------------------------------------------------------
		// Grid Rendering
		// ----------------------------------------------------------------------------

		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

		if (tileVis) {

			colorProgram.use();
			colorProgram.setMat4("ModelMatrix", identityMatrix);
			colorProgram.setMat4("ViewMatrix", view);
			colorProgram.setMat4("ProjectionMatrix", projection);
			colorProgram.setVec4("Color", glm::vec4(1.0f, 1.0f, 0.0f, 1.0f));

			glBindVertexArray(debugPositionsVertexArray);
			glDrawArrays(GL_LINES, 0, 24 * numTilesX * numTilesY);
			glBindVertexArray(0);
		}

		// ----------------------------------------------------------------------------
		// Draw Light Sources
		// ----------------------------------------------------------------------------

		if (lightVis) {
			uniformColorProgram.use();
			uniformColorProgram.setMat4("ViewMatrix", view);
			uniformColorProgram.setMat4("ProjectionMatrix", projection);

			glBindBuffer(GL_SHADER_STORAGE_BUFFER, lightsBuffer);

			PointLight* lightsPointer = (PointLight*) glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);

			for (int lightIndex = 0; lightIndex < NR_LIGHTS; ++lightIndex) {
				PointLight light = lightsPointer[lightIndex];
				glm::mat4 model(1.0f);
				model = glm::translate(model, light.position);
				model = glm::scale(model, glm::vec3(0.05f));

				uniformColorProgram.setVec4("Color", light.color);
				uniformColorProgram.setMat4("ModelMatrix", model);
				renderSphere();
			}
			glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
		}

		// ----------------------------------------------------------------------------
		// Rendering of the scene
		// ----------------------------------------------------------------------------

		glm::mat4 model(1.0f);
		model = glm::scale(model, glm::vec3(0.005f));
		glm::mat3 normal = glm::inverseTranspose(glm::mat3(model));
		mainProgram.use();
		mainProgram.setMat4("ViewMatrix", view);
		mainProgram.setMat4("ProjectionMatrix", projection);
		mainProgram.setInt("NumLights", NR_LIGHTS);
		mainProgram.setInt2("TileSize", TILE_SIZE, TILE_SIZE);
		mainProgram.setInt2("NumTiles", numTilesX, numTilesY);
		mainProgram.setInt("RenderHeatmap", heatmapVis);
		mainProgram.setMat4("ModelMatrix", model);
		mainProgram.setMat4("NormalMatrix", normal);

		// Synchronization direkt vor dem Rendern des Modells, damit der Compute Shader so lange wie möglich unabhängig von dem
		// Draw Command arbeiten kann.
		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
		object.Draw(mainProgram);

		glEndQuery(GL_TIME_ELAPSED);

		// check and print timer query
		GLuint64 elapsed_time_in_nanoseconds;

		//GLint available = 0;
		//while (!available) {
		//	glGetQueryObjectiv(timerQuery[(frameIndex + 1) % 2], GL_QUERY_RESULT_AVAILABLE, &available);
		//}

		glGetQueryObjectui64v(timerQuery[(frameIndex + 1) % 2], GL_QUERY_RESULT, &elapsed_time_in_nanoseconds);

		double time = static_cast<double>(elapsed_time_in_nanoseconds) / 1000000.0;
		// print render time every 60 frames
		if (frameIndex%60 == 0) {
			std::cout << "Rendering time: " << time << "ms | " << "FPS: " << 1.0 / time * 1000.0 << std::endl;
		}
		
		++frameIndex;

		// reset update grid 
		updateGrid = false;

		// glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
		// -------------------------------------------------------------------------------
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	// optional: de-allocate all resources once they've outlived their purpose:
	// ------------------------------------------------------------------------
	glDeleteQueries(2, timerQuery);
	glDeleteBuffers(1, &visibleLightsBuffer);

	glDeleteBuffers(1, &lightsBuffer);
	glDeleteBuffers(1, &debugColors);
	glDeleteBuffers(1, &debugPositions);
	glDeleteVertexArrays(1, &debugPositionsVertexArray);

	// glfw: terminate, clearing all previously allocated GLFW resources.
	// ------------------------------------------------------------------
	glfwTerminate();
	return 0;
}

// renderSphere() renders a 1x1 3D sphere in NDC.
// -------------------------------------------------
GLuint sphereVAO = 0;
GLuint indexCount;

void renderSphere()
{
	// initialize (if necessary)
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

		const unsigned int X_SEGMENTS = 64;
		const unsigned int Y_SEGMENTS = 64;
		const float PI = 3.14159265359f;
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
					indices.push_back(y       * (X_SEGMENTS + 1) + x);
					indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
				}
			}
			else
			{
				for (int x = X_SEGMENTS; x >= 0; --x)
				{
					indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
					indices.push_back(y       * (X_SEGMENTS + 1) + x);
				}
			}
			oddRow = !oddRow;
		}
		indexCount = (GLuint)indices.size();

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
		GLsizei stride = (3 + 2 + 3) * sizeof(float);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, stride, (void*)(5 * sizeof(float)));
	}
	// render Cube
	glBindVertexArray(sphereVAO);
	glDrawElements(GL_TRIANGLE_STRIP, indexCount, GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);
}

/// <summary>
/// Repares the tiles memory.
/// create buffer with indices of visible light sources
/// create memory for the light indices per tiles  
/// </summary>
/// <param name="width">The framebuffer width.</param>
/// <param name="height">The framebuffer height.</param> 
/// <param name="numLights">The number of lights.</param> 
void repareTilesMemory(int width, int height) {
	int numTilesX = (width + (TILE_SIZE - 1)) / TILE_SIZE;
	int numTilesY = (height + (TILE_SIZE - 1)) / TILE_SIZE;

	int numTiles = numTilesX * numTilesY;

	// free buffer
	if (visibleLightsBuffer != NULL) {
		glDeleteBuffers(1, &visibleLightsBuffer);
	}
	// create new buffer 
	glGenBuffers(1, &visibleLightsBuffer);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, visibleLightsBuffer);
	// +1 since the number of light sources per tiles will be save in front of the indice
	glBufferData(GL_SHADER_STORAGE_BUFFER, (NR_LIGHTS + 1) * numTiles * sizeof(GLuint), NULL, GL_DYNAMIC_DRAW);

	std::cout << "Create Visible Lights Buffer Lights: " << NR_LIGHTS << " Tiles: " << numTiles << std::endl;

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

	if (glfwGetKey(window, GLFW_KEY_H) == GLFW_PRESS) {
		h_pressed = true;
	}
	if (glfwGetKey(window, GLFW_KEY_H) == GLFW_RELEASE && h_pressed) {
		std::cout << "Help:" << std::endl;
		std::cout << " W: Toggle Wireframe" << std::endl;
		std::cout << " M: Toggle Heatmap" << std::endl;
		std::cout << " T: Toggle Show Tiles" << std::endl;
		std::cout << " L: Toggle Show Lights" << std::endl;
		h_pressed = false;
	}

	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
		w_pressed = true;
	}
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_RELEASE && w_pressed) {
		wireframe = !wireframe;
		w_pressed = false;
	}

	if (glfwGetKey(window, GLFW_KEY_M) == GLFW_PRESS) {
		m_pressed = true;
	}
	if (glfwGetKey(window, GLFW_KEY_M) == GLFW_RELEASE && m_pressed) {
		heatmapVis = !heatmapVis;
		m_pressed = false;
	}

	if (glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS) {
		t_pressed = true;
	}
	if (glfwGetKey(window, GLFW_KEY_T) == GLFW_RELEASE && t_pressed) {
		tileVis = !tileVis;
		t_pressed = false;
	}

	if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS) {
		l_pressed = true;
	}
	if (glfwGetKey(window, GLFW_KEY_L) == GLFW_RELEASE && l_pressed) {
		lightVis = !lightVis;
		l_pressed = false;
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

		double xoffset = xpos - lastX;
		double yoffset = lastY - ypos;
		// reversed since y-coordinates range from bottom to top
		lastX = xpos;
		lastY = ypos;

		camera.rotate((float)xoffset, (float)yoffset);

		// update grid on mouse change
		updateGrid = true;
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

		double xoffset = xpos - lastX;
		double yoffset = lastY - ypos;
		// reversed since y-coordinates range from bottom to top
		lastX = xpos;
		lastY = ypos;

		//camera.ProcessMouseMovement(xoffset, yoffset);
		if (xoffset > 0) {
			camera.translate(LEFT, (float)xoffset*0.05f);
		}
		else {
			camera.translate(RIGHT, (float)xoffset*-0.05f);
		}

		if (yoffset > 0) {
			camera.translate(DOWN, (float)yoffset*0.05f);
		}
		else {
			camera.translate(UP, (float)yoffset*-0.05f);
		}

		// update grid on mouse change
		updateGrid = true;
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

		double xoffset = xpos - lastX;
		//float yoffset = lastY - ypos;
		// reversed since y-coordinates range from bottom to top
		lastX = xpos;
		//lastY = ypos;

		//camera.ProcessMouseMovement(xoffset, yoffset);
		if (xoffset > 0) {
			camera.translate(FORWARD, (float)xoffset*0.05f);
		}
		else {
			camera.translate(BACKWARD, (float)xoffset*-0.05f);
		}
		// update grid on mouse change
		updateGrid = true;
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
	(void)window;
	(void)xoffset;
	camera.ProcessMouseScroll((float)yoffset);
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	(void)window;
	// save framebuffer width and height
	framebufferHeight = height;
	framebufferWidth = width;

	// make sure the viewport matches the new window dimensions; note that width and 
	// height will be significantly larger than specified on retina displays.
	glViewport(0, 0, width, height);

	repareTilesMemory(width, height);

	// update grid on viewport change
	updateGrid = true;
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