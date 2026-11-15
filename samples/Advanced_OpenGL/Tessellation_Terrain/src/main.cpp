/*
 * Tessellation and vertex-based terrain displacement
 *
 * Tessellation adds two programmable shader stages and one fixed-function
 * primitive generator. Control points are grouped into patches. The
 * tessellation control shader (TCS) optionally selects tessellation levels and
 * produces an output patch; the tessellation evaluation shader (TES) evaluates
 * every generated vertex. A TES is therefore required, while a TCS is optional.
 *
 * This sample uses view-dependent subdivision: nearby terrain patches receive
 * more triangles than distant patches. Matching outer levels on shared edges
 * prevents cracks between neighboring patches.
 */

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "stb_image.h"

#include "modules/shader_m.h"
#include "modules/camera.h"
#include "modules/filesystem.h"
#include "modules/window.h"
#include "modules/help_overlay.h"

#include <iostream>
#include <string>

// callbacks
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
// current Mouse position 
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow *window);

/* load texture */
unsigned int loadTexture(const char *path, bool gammaCorrection);

struct TerrainMesh {
	unsigned int VAO = 0;
	unsigned int VBO = 0;
	unsigned int EBO = 0;
	unsigned int numStrips = 0;
	unsigned int indicesPerStrip = 0;
};

TerrainMesh createTerrain(int width, int height, int nrChannels, const stbi_us* data);

// settings
const unsigned int SCR_WIDTH = 1280;
const unsigned int SCR_HEIGHT = 720;

const unsigned int NUM_PATCH_PTS = 4;

bool Wpressed = false;
bool Fpressed = false;
bool Tpressed = false;
bool wireframe = false;
bool useTessellation = true;
int displayMode = 0;

// camera
Camera camera(glm::vec3(67.0f, 627.5f, 169.9f),
	glm::vec3(0.0f, 1.0f, 0.0f),
	-128.1f, -42.4f);
float lastX = SCR_WIDTH / 2.0;
float lastY = SCR_HEIGHT / 2.0;
bool firstMouse = true;
bool showFPS = true;

// timing 
float deltaTime = 0.0f; // time between current frame and last frame
float lastFrame = 0.0f; // time of last frame

int main()
{
	// glfw: initialize and configure
	// ------------------------------
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

	#ifdef __APPLE__
		glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // uncomment this statement to fix compilation on OS X
	#endif

	// glfw window creation
	// --------------------
	GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Tessellation Terrain | measuring FPS...", NULL, NULL);
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
		{"F", "Toggle FPS display"},
		{"W", "Toggle wireframe"},
		{"T", "Tessellated / mesh terrain"},
		{"1 / 2", "Height / normal view"}
	});
	GLint maxTessLevel;
	glGetIntegerv(GL_MAX_TESS_GEN_LEVEL, &maxTessLevel);
	std::cout << "Max available tess level: " << maxTessLevel << std::endl;

	// configure global OpenGL state
	// -----------------------------
	/* DEPTH BUFFER */
	glEnable(GL_DEPTH_TEST);

	// build and compile shader program(s)
	// ------------------------------------
	Shader tessellationShader(FileSystem::getSamplePath("shader/tessellationShader.vert").c_str(), FileSystem::getSamplePath("shader/tessellationShader.frag").c_str(), nullptr, FileSystem::getSamplePath("shader/tessellationShader.tesc").c_str(), FileSystem::getSamplePath("shader/tessellationShader.tese").c_str());
	Shader meshShader(FileSystem::getSamplePath("shader/vertexShader.vert").c_str(), FileSystem::getSamplePath("shader/fragmentShader.frag").c_str());
	// load textures
	// -------------
	// read in height map data
	stbi_set_flip_vertically_on_load(true);
	unsigned int texture = loadTexture(FileSystem::getPath("content/images/iceland_heightmap.png").c_str(), false);

	int width, height, nrChannels;
	stbi_us* data = stbi_load_16(FileSystem::getPath("content/images/iceland_heightmap.png").c_str(), &width, &height, &nrChannels, 0);
	if (!data)
	{
		std::cout << "Failed to load heightmap" << std::endl;
		glDeleteTextures(1, &texture);
		sample_help::shutdown();
		glfwTerminate();
		return -1;
	}
	std::cout << "Loaded heightmap of size " << width << " x " << height << std::endl;

	tessellationShader.use();
	tessellationShader.setInt("heightMap", 0);
	tessellationShader.setVec2("texelSize", glm::vec2(1.0f / width, 1.0f / height));

	// set up vertex data (and buffer(s)) and configure vertex attributes
	// ------------------------------------------------------------------
	std::vector<float> vertices;


	// input rez of grid/terrain will be 
	unsigned int rez = 20; // generating rez*rez patches

	// reserve data
	vertices.reserve(rez * rez * (3+2)*4);

	for (unsigned int i = 0; i <= rez - 1; i++)
	{
		for (unsigned int j = 0; j <= rez - 1; j++)
		{
			vertices.push_back(-height / 2.0f + height * j / (float)rez); // v.x
			vertices.push_back(0.0f); // v.y
			vertices.push_back(-width / 2.0f + width * i / (float)rez); // v.z
			vertices.push_back(i / (float)rez); // u
			vertices.push_back(j / (float)rez); // v

			vertices.push_back(-height / 2.0f + height * j / (float)rez); // v.x
			vertices.push_back(0.0f); // v.y
			vertices.push_back(-width / 2.0f + width * (i + 1) / (float)rez); // v.z
			vertices.push_back((i + 1) / (float)rez); // u
			vertices.push_back(j / (float)rez); // v

			vertices.push_back(-height / 2.0f + height * (j + 1) / (float)rez); // v.x
			vertices.push_back(0.0f); // v.y
			vertices.push_back(-width / 2.0f + width * i / (float)rez); // v.z
			vertices.push_back(i / (float)rez); // u
			vertices.push_back((j + 1) / (float)rez); // v

			vertices.push_back(-height / 2.0f + height * (j + 1) / (float)rez); // v.x
			vertices.push_back(0.0f); // v.y
			vertices.push_back(-width / 2.0f + width * (i + 1) / (float)rez); // v.z
			vertices.push_back((i + 1) / (float)rez); // u
			vertices.push_back((j + 1) / (float)rez); // v
		}
	}
	std::cout << "Loaded " << rez * rez << " patches of 4 control points each" << std::endl;
	std::cout << "Processing " << rez * rez * 4 << " vertices in vertex shader" << std::endl;

	// first, configure the cube's VAO (and terrainVBO)
	unsigned int terrainVAO, terrainVBO;
	glGenVertexArrays(1, &terrainVAO);
	glBindVertexArray(terrainVAO);

	glGenBuffers(1, &terrainVBO);
	glBindBuffer(GL_ARRAY_BUFFER, terrainVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * vertices.size(), &vertices[0], GL_STATIC_DRAW);

	// position attribute
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	// texCoord attribute
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(sizeof(float) * 3));
	glEnableVertexAttribArray(1);

	// specify number of vertices that make up each of the primitives 
	// Patch --> abstract primitive compromised of a set of n vertices
	glPatchParameteri(GL_PATCH_VERTICES, NUM_PATCH_PTS);

	// create vertices matching width and height of the loaded texture
	TerrainMesh terrain = createTerrain(width, height, nrChannels, data);

	// release the image data
	stbi_image_free(data);

	int frameCount = 0;
	double previousTime = glfwGetTime();
	// render loop
	// -----------
	while (!glfwWindowShouldClose(window))
	{
		// wireframe
		glPolygonMode(GL_FRONT_AND_BACK, (wireframe) ? GL_LINE : GL_FILL);

		glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK);

		// Set frame time
		GLfloat currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame; // time per frame 
		lastFrame = currentFrame;

		// Update the window title once per second to keep the FPS readable.
		if (showFPS) {
			frameCount++;
			const double elapsed = currentFrame - previousTime;
			if (elapsed >= 1.0)
			{
				const int framesPerSecond = static_cast<int>(frameCount / elapsed + 0.5);
				const std::string title = "Tessellation Terrain | " + std::to_string(framesPerSecond) + " FPS";
				glfwSetWindowTitle(window, title.c_str());
				frameCount = 0;
				previousTime = currentFrame;
			}
		} else {
			frameCount = 0;
			previousTime = currentFrame;
		}

		// Check and call events
		processInput(window);

		// render
		// ------
		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (GLfloat)SCR_WIDTH / (GLfloat)SCR_HEIGHT, 0.1f, 100000.0f);
		glm::mat4 model(1.0f);
		
		// render the terrain 
		if (useTessellation)
		{
			tessellationShader.use();
			tessellationShader.setMat4("projection", projection);
			tessellationShader.setMat4("view", camera.GetViewMatrix());
			tessellationShader.setMat4("model", model);
			tessellationShader.setInt("displayMode", displayMode);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, texture);
			glBindVertexArray(terrainVAO);
			glDrawArrays(GL_PATCHES, 0, NUM_PATCH_PTS * rez * rez);
		}
		else
		{
			meshShader.use();
			meshShader.setMat4("projection", projection);
			meshShader.setMat4("view", camera.GetViewMatrix());
			meshShader.setMat4("model", model);
			meshShader.setInt("displayMode", displayMode);
			glBindVertexArray(terrain.VAO);
			for (unsigned int strip = 0; strip < terrain.numStrips; strip++)
			{
				glDrawElements(GL_TRIANGLE_STRIP,
					terrain.indicesPerStrip,
					GL_UNSIGNED_INT,
					(void*)(sizeof(unsigned int) * terrain.indicesPerStrip * strip));
			}
		}
		// glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
		// -------------------------------------------------------------------------------
		sample_help::render();
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	// optional: de-allocate all resources once they've outlived their purpose:
	// ------------------------------------------------------------------------
	glDeleteVertexArrays(1, &terrainVAO);
	glDeleteBuffers(1, &terrainVBO);
	glDeleteVertexArrays(1, &terrain.VAO);
	glDeleteBuffers(1, &terrain.VBO);
	glDeleteBuffers(1, &terrain.EBO);
	glDeleteTextures(1, &texture);
	
	// glfw: terminate, clearing all previously allocated GLFW resources.
	// ------------------------------------------------------------------
	sample_help::shutdown();
	glfwTerminate();
	return 0;
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

	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
		Wpressed = true;
	}

	if (Wpressed && glfwGetKey(window, GLFW_KEY_W) == GLFW_RELEASE) {
		Wpressed = false;
		wireframe = !wireframe;
	}

	if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS) {
		Fpressed = true;
	}

	if (Fpressed && glfwGetKey(window, GLFW_KEY_F) == GLFW_RELEASE) {
		Fpressed = false;
		showFPS = !showFPS;
		glfwSetWindowTitle(window, showFPS
			? "Tessellation Terrain | measuring FPS..."
			: "Tessellation Terrain");
	}

	if (glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS) {
		Tpressed = true;
	}

	if (Tpressed && glfwGetKey(window, GLFW_KEY_T) == GLFW_RELEASE) {
		Tpressed = false;
		useTessellation = !useTessellation;
		std::cout << "Terrain renderer: " << (useTessellation ? "GPU tessellation" : "CPU mesh") << std::endl;
	}

	if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS) {
		displayMode = 0;
	}
	else if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS) {
		displayMode = 1;
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
			firstMouse = false;
		}

		float xoffset = xpos - lastX;
		lastX = xpos;

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

TerrainMesh createTerrain(int width, int height, int nrChannels, const stbi_us* data) {
	std::vector<float> vertices;
	vertices.reserve(static_cast<size_t>(height) * width * 3);

	const unsigned int channels = nrChannels;
	for (int i = 0; i < height; i++)
	{
		for (int j = 0; j < width; j++)
		{
			const stbi_us heightValue = data[(j + width * i) * channels];
			vertices.push_back(-width / 2.0f + j);
			vertices.push_back(static_cast<float>(heightValue) / 65535.0f * 64.0f - 16.0f);
			vertices.push_back(-height / 2.0f + i);
		}
	}
	std::cout << "Loaded " << vertices.size() / 3 << " mesh vertices" << std::endl;

	// create mesh as triangle stripes
	std::vector<unsigned int> indices;
	for (unsigned int i = 0; i < height - 1; i += 1)
	{
		for (unsigned int j = 0; j < width; j += 1)
		{
			for (unsigned int k = 0; k < 2; k++)
			{
				indices.push_back(j + width * (i + k));
			}
		}
	}
	std::cout << "Loaded " << indices.size() << " indices" << std::endl;

	TerrainMesh terrain;
	terrain.numStrips = height - 1;
	terrain.indicesPerStrip = width * 2;
	std::cout << "Created lattice of " << terrain.numStrips << " strips with "
		<< terrain.indicesPerStrip - 2 << " triangles each" << std::endl;

	glGenVertexArrays(1, &terrain.VAO);
	glGenBuffers(1, &terrain.VBO);
	glGenBuffers(1, &terrain.EBO);
	glBindVertexArray(terrain.VAO);
	glBindBuffer(GL_ARRAY_BUFFER, terrain.VBO);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, terrain.EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
	glBindVertexArray(0);

	return terrain;
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
	 * Specular and normal maps usually contain linear data. Loading them as sRGB corrupts the lighting calculations.
	 */
	unsigned int textureID;
	glGenTextures(1, &textureID);

	int width, height, nrComponents;
	stbi_us *data = stbi_load_16(path, &width, &height, &nrComponents, 0);
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
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // support non-power-of-two heightmap textures
		glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, dataFormat, GL_UNSIGNED_SHORT, data);
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
