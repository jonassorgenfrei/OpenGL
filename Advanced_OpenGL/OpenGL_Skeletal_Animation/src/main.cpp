/* 
 *	Tesselation
 *		Vertex-basiertes Displacement Mapping
 *		2 Shader und eine Fixed-Function Stufe => unterteilung von Patches
 *		Es muss mind. der Tessellation Evaluation Shader implementiert werden.
 *		=> dynamische Erzeugung von Geometrien (nur Zusammenhangend)
 *		Ausgabe lasst sich nicht auf vers. Layer/Viewports umleiten
 *		Erstellung auf Grund von Kontrollpunkten, die in Patches organisiert sind
 *		TYPISCH: Betrachterabhangige Unterteilung von Dreiecken.
 *
 *		Datenfluss:
 *			ohne TCS : Tessllationsstaerke durch Standardwerte bestimmt
 *			mit TCS : TCS kann Tessllationsstraerke steuern Vertex - Anzahlen muessen nicht uebereinstimmen(Spline Flachen)
 *
 *		Tessellation Primitive Generation Stufe 
 *			Fixed-Function Stuffe - kann Menge von Primitiven nach einem vorgegebenen Muster erzeugen 
 *			Arbeitet auf abstraketem Path
 *
 *		Die St‰rke der Tessellation kann innerhalb und an den Auﬂenkanten des Patches seperat gesteuert werden.
 */

// USE TESSELLATION SHADER
#define USE_TESSELLATION 1


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
unsigned int loadTexture(const char *path, bool gammaCorrection);
Mesh createTerrain(int width, int height, int nrChannels, stbi_us* data);
void icon(GLFWwindow* window);


// settings
const unsigned int SCR_WIDTH = 1280;
const unsigned int SCR_HEIGHT = 720;

#if USE_TESSELLATION
const unsigned int NUM_PATCH_PTS = 4;
#endif

bool Wpressed = false;
bool Fpressed = false;
bool wireframe = false;

// camera
Camera camera(glm::vec3(67.0f, 627.5f, 169.9f),
	glm::vec3(0.0f, 1.0f, 0.0f),
	-128.1f, -42.4f);
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
#if USE_TESSELLATION
	GLint maxTessLevel;
	glGetIntegerv(GL_MAX_TESS_GEN_LEVEL, &maxTessLevel);
	std::cout << "Max available tess level: " << maxTessLevel << std::endl;
#endif

	// configure global opengl state
	// -----------------------------
	/* DEPTH BUFFER */
	glEnable(GL_DEPTH_TEST);

	// build and compile shader program(s)
	// ------------------------------------
#if USE_TESSELLATION
	Shader shader(FileSystem::getPath("shader/tessellationShader.vert").c_str(), FileSystem::getPath("shader/tessellationShader.frag").c_str(), NULL, FileSystem::getPath("shader/tessellationShader.tesc").c_str(), FileSystem::getPath("shader/tessellationShader.tese").c_str());
#else
	Shader shader(FileSystem::getPath("shader/vertexShader.vert").c_str(), FileSystem::getPath("shader/fragmentShader.frag").c_str());
#endif
	// load textures
	// -------------
	// read in height map data
#if USE_TESSELLATION
	unsigned int texture = loadTexture(FileSystem::getPath("../../content/images/iceland_heightmap.png").c_str(), false);

	int width = 2624;
	int height = 1756;

	shader.use();
	shader.setInt("heightMap", 0);
	shader.setVec2("texelSize", glm::vec2(1.0f/width, 1.0f/height));

#else
	stbi_set_flip_vertically_on_load(true);
	int width, height, nrChannels;
	stbi_us* data = stbi_load_16(FileSystem::getPath("../../content/images/iceland_heightmap.png").c_str(), &width, &height, &nrChannels, 0);
	if (data)
	{
		std::cout << "Loaded heightmap of size " << height << " x " << width << std::endl;
	}
	else
	{
		std::cout << "Failed to load texture" << std::endl;
	}
#endif

	// set up vertex data (and buffer(s)) and configure vertex attributes
	// ------------------------------------------------------------------
#if USE_TESSELLATION
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
#else
	// create vertices matching width and height of the loaded texture
	Mesh terrain = createTerrain(width, height, nrChannels, data);

	// release the image data
	stbi_image_free(data);

	const int numStrips = (height - 1) ;
	const int numTrisPerStrip = (width) * 2 - 2;
#endif

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

		// Check and call events
		processInput(window);

		// render
		// ------
		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		shader.use();
		glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (GLfloat)SCR_WIDTH / (GLfloat)SCR_HEIGHT, 0.1f, 100000.0f);
		shader.setMat4("projection", projection);
		shader.setMat4("view", camera.GetViewMatrix());

		glm::mat4 model(1.0f);
		//model = glm::rotate(model, glm::radians((float)glfwGetTime() * -10.0f), glm::normalize(glm::vec3(1.0, 0.0, 1.0)));
		shader.setMat4("model", model);
		
		// render the terrain 
#if USE_TESSELLATION
		glBindVertexArray(terrainVAO);
		// draw patches
		// patches will be size of 4 and amount will be mutliplied by rez*rez 
		glDrawArrays(GL_PATCHES, 0, NUM_PATCH_PTS* rez* rez);
#else
		glBindVertexArray(terrain.VAO);
		
		// height -1 draw calls
		for (unsigned int strip = 0; strip < numStrips; strip++)
		{
			glDrawElements(GL_TRIANGLE_STRIP,   // primitive type
				numTrisPerStrip + 2,   // number of indices to render
				GL_UNSIGNED_INT,     // index data type
				(void*)(sizeof(unsigned int) * (numTrisPerStrip + 2) * strip)); // offset to starting index
		}
#endif
		// glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
		// -------------------------------------------------------------------------------
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	// optional: de-allocate all resources once they've outlived their purpose:
	// ------------------------------------------------------------------------
#if USE_TESSELLATION
	glDeleteVertexArrays(1, &terrainVAO);
	glDeleteBuffers(1, &terrainVBO);
#endif
	
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
		if (!printFPS) {
			std::cout << "PRINT FPS" << std::endl;
		}

		printFPS = !printFPS;
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

Mesh createTerrain(int width, int height, int nrChannels, stbi_us* data) {

	// time intensive O(n2) & memory intensive ~72mb 
	// fixed resolution
	std::vector<Vertex> vertices;
	// reserve data
	vertices.reserve(height * width*sizeof(Vertex));

	float yScale = 64.0f / 256.0f, yShift = 16.0f;
	unsigned int bytePerPixel = nrChannels;
	for (int i = 0; i < height; i++)
	{
		for (int j = 0; j < width; j++)
		{
			stbi_us* texel = data + (j + width * i) * bytePerPixel;	// calculating the pointer offset
			const stbi_us y = *texel; // derefernce the pointer to a value, use first channel, convert short to 

			Vertex vertex;

			vertex.Position.x = -width / 2.0f + j;   // vx
			vertex.Position.y = (float)y*0.0025f * yScale - yShift;   // vy
			vertex.Position.z = -height / 2.0f + i;   // vz

			vertices.push_back(vertex);
		}
	}
	std::cout << "Loaded " << vertices.size() / 3 << " vertices" << std::endl;

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

	const int numStrips = (height - 1);
	const int numTrisPerStrip = (width) * 2 - 2;
	std::cout << "Created lattice of " << numStrips << " strips with " << numTrisPerStrip << " triangles each" << std::endl;
	std::cout << "Created " << numStrips * numTrisPerStrip << " triangles total" << std::endl;

	std::vector<Texture> tex = std::vector<Texture>();

	return Mesh(vertices, indices, tex);
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