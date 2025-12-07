/* 
 *	Billboards
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
#include "modules/window.h"

#include <iostream>


// callbacks
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
// current Mouse position 
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow *window);

/* load texture */
unsigned int loadTexture(const char *path, bool gammaCorrection);
unsigned int loadTextureArray(vector<std::string> paths, bool gammaCorrection);
float heightFunc(glm::vec2 pos);
Mesh initTriangles();

// settings
const unsigned int SCR_WIDTH = 1920;
const unsigned int SCR_HEIGHT = 1080;

bool Wpressed = false;
bool Fpressed = false;
bool Upressed = false;
bool Apressed = false;

// flags
bool wireframe = false;
int subdivs = 4;
int billboards = 2;
bool animation = true;
bool showTexCoords = false;

float alphaThreshold = 0.5f;

// camera
Camera camera(glm::vec3(-0.1f, 1.1f, 4.3f), glm::vec3(0,1,0), -90, -21);
float lastX = SCR_WIDTH / 2.0;
float lastY = SCR_HEIGHT / 2.0;
bool firstMouse = true;
bool printFPS = false;


#define VERTEX_ATTRIB_POSITION 0            /**< Attribute Location für die Positionen */
#define VERTEX_ATTRIB_NORMAL   1            /**< Attribute Location für die Normalen */
#define VERTEX_ATTRIB_TEXCOORD 2            /**< Attribute Location für die Textur Koordinaten */

#define SIZE 32

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


	// configure global opengl state
	// -----------------------------
	/* DEPTH BUFFER */
	glEnable(GL_DEPTH_TEST);

	// build and compile shader program(s)
	// main
	Shader groundShader(FileSystem::getSamplePath("shader/ground.vert").c_str(), FileSystem::getSamplePath("shader/ground.frag").c_str());
	Shader grassShader(FileSystem::getSamplePath("shader/billboards.vert").c_str(), FileSystem::getSamplePath("shader/billboards.frag").c_str(), FileSystem::getSamplePath("shader/billboards.geom").c_str());

	// load textures
	// -------------
	// read in height map data
	unsigned int ground = loadTexture(FileSystem::getPath("content/images/ground.jpg").c_str(), false);
	unsigned int grass = loadTexture(FileSystem::getPath("content/images/grass.png").c_str(), false);
	
	vector<std::string> paths;
	paths.push_back(FileSystem::getPath("content/images/grass.png"));
	paths.push_back(FileSystem::getPath("content/images/grass2.png"));
	paths.push_back(FileSystem::getPath("content/images/billboardblueflowers.png"));
	paths.push_back(FileSystem::getPath("content/images/billboardredflowers2.png"));

	unsigned int grassArray = loadTextureArray(paths, false);

	// set up vertex data (and buffer(s)) and configure vertex attributes
	// ------------------------------------------------------------------
	
	Mesh triangles = initTriangles();


	int frameCount = 0;
	double previousTime = glfwGetTime();

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

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
				std::cout << camera.Position.x << camera.Position.y << camera.Position.z << std::endl;
				std::cout << camera.Up.x << camera.Up.y << camera.Up.z << std::endl;
				std::cout << camera.Yaw << camera.Pitch << std::endl;
			}
		} else {
			frameCount = 0;
			previousTime = glfwGetTime();
		}

		// Check and call events
		processInput(window);

		// render
		// ------

		glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (GLfloat)SCR_WIDTH / (GLfloat)SCR_HEIGHT, 0.01f, 100.0f);
		glm::mat4 model(1.0f);
		//model = glm::rotate(model, glm::radians((float)glfwGetTime() * -10.0f), glm::normalize(glm::vec3(1.0, 0.0, 1.0)));

		// ground
		glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
		glClearDepth(1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


		groundShader.use();
		groundShader.setMat4("projection", projection);
		groundShader.setMat4("view", camera.GetViewMatrix());
		groundShader.setMat4("model", model);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, ground);
		groundShader.setInt("tex", 0);
		
		triangles.Draw(groundShader);

		// grass
		grassShader.use();
		grassShader.setMat4("projection", projection);
		grassShader.setMat4("view", camera.GetViewMatrix());
		grassShader.setMat4("model", model);
		grassShader.setFloat("NormalLength", 0.5f);
		grassShader.setInt("Subdivs", subdivs);
		grassShader.setInt("Billboards", billboards);
		grassShader.setFloat("alphaThreshold", alphaThreshold);
		grassShader.setFloat("Time", currentFrame);
		grassShader.setBool("Animation", animation);
		grassShader.setBool("DrawTexCoords", showTexCoords);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D_ARRAY, grassArray);
		grassShader.setInt("texArray", 0);
		// render opaque
		triangles.Draw(groundShader);

		// render transparent
		grassShader.setFloat("alphaThreshold", 0.0f);
		glDepthMask(GL_FALSE);
		glEnable(GL_BLEND);

		triangles.Draw(groundShader);

		glDisable(GL_BLEND);
		glDepthMask(GL_TRUE);

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

	if (glfwGetKey(window, GLFW_KEY_U) == GLFW_PRESS) {
		Upressed = true;
	}

	if (Upressed && glfwGetKey(window, GLFW_KEY_U) == GLFW_RELEASE) {
		Upressed = false;

		showTexCoords = !showTexCoords;
	}

	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
		Apressed = true;
	}

	if (Apressed && glfwGetKey(window, GLFW_KEY_A) == GLFW_RELEASE) {
		Apressed = false;

		animation = !animation;
	}

	if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
		alphaThreshold = min(1.0f, alphaThreshold + 0.1f);
	}

	if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
		alphaThreshold = max(0.0f, alphaThreshold - 0.1f);
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

float heightFunc(glm::vec2 pos) {
	srand(1);
	const int functionCount = 128;
	float offset[functionCount * 2];
	float amplitudes[functionCount * 2];
	float frequenzies[functionCount * 2];

	for (int i = 0; i < 64; i++) {

		offset[i] = (rand() / (float)INT_MAX) * glm::pi<float>();
		amplitudes[i] = rand() / (float)INT_MAX;
		frequenzies[i] = rand() / (float)INT_MAX;
	}

	float result = 0;
	float frequenz = 0.25;
	for (int i = 0; i < 32; i++) {
		result += sin(pos.x * frequenz * frequenzies[i * 2] + offset[i * 2]) * (1 / frequenz) * amplitudes[i * 2] +
			sin(pos.y * frequenz * frequenzies[i * 2] + offset[i * 2 + 1]) * (1 / frequenz) * amplitudes[i * 2 + 1];
		frequenz *= 2;
	}
	return result * 0.25 - 1.;
}

Mesh initTriangles() {
	std::vector<Vertex> vertices;
	// reserve data
	vertices.reserve(SIZE * SIZE * sizeof(Vertex));

	// create mesh as triangle
	std::vector<unsigned int> indices;
	indices.reserve((SIZE - 1) * (SIZE - 1) * 6 * sizeof(unsigned int));

	for (int y = 0; y < SIZE; y++) {
		for (int x = 0; x < SIZE; x++) {
			int i = y * SIZE + x;
			Vertex v;
			v.Position = glm::vec3((((float)x / SIZE) * 2 - 1) * SIZE / 16, 0, (((float)y / SIZE) * 2 - 1) * SIZE / 16);
			v.Position.y = heightFunc(glm::vec2(v.Position.x, v.Position.z));
			v.TexCoords = glm::vec2(((float)x / SIZE), (float)y / SIZE);

			vertices.push_back(v);
		}
	}

	for (int y = 0; y < SIZE; y++) {
		for (int x = 0; x < SIZE; x++) {
			glm::vec3 pr = x < SIZE - 1 ? vertices[y * SIZE + (x + 1)].Position : vertices[y * SIZE + x].Position;
			glm::vec3 pl = x > 0 ? vertices[y * SIZE + (x - 1)].Position : vertices[y * SIZE + x].Position;
			glm::vec3 pt = y < SIZE - 1 ? vertices[(y + 1) * SIZE + x].Position : vertices[y * SIZE + x].Position;
			glm::vec3 pb = y > 0 ? vertices[(y - 1) * SIZE + x].Position : vertices[y * SIZE + x].Position;
			glm::vec3 v1 = pr - pl;
			glm::vec3 v2 = pt - pb;
			vertices[y * SIZE + x].Normal = glm::normalize(glm::cross(v2, v1));
		}
	}

	std::cout << "Loaded " << vertices.size() / 3 << " vertices" << std::endl;
		
	for (int y = 0; y < SIZE - 1; y++) {
		for (int x = 0; x < SIZE - 1; x++) {
			int i = (y * (SIZE - 1) + x) * 6;
			//first triangle
			indices.push_back((y + 1) * SIZE + x);
			indices.push_back(y * SIZE + x + 1);
			indices.push_back(y * SIZE + x);
			
			//second triangle
			indices.push_back((y + 1) * SIZE + x);
			indices.push_back((y + 1) * SIZE + x + 1);
			indices.push_back(y * SIZE + x + 1);
		}
	}
	std::cout << "Loaded " << indices.size() << " indices" << std::endl;

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


unsigned int loadTextureArray(vector<std::string> paths, bool gammaCorrection) {
	unsigned int textureID = NULL;
	
	int width, height, nrComponents;
	int i = 0;
	for(std::string path: paths) {
		int w, h, c;
		stbi_us* data = stbi_load_16(path.c_str(), &w, &h, &c, 0);
		if (data)
		{
			GLenum internalFormat;
			GLenum dataFormat;
			if (c == 1) {
				internalFormat = dataFormat = GL_RED;
			}
			else if (c == 3) {
				internalFormat = (gammaCorrection) ? GL_SRGB : GL_RGB;	// GL_SRGB => OpenGL will correct image to linear color space
				dataFormat = GL_RGB;
			}
			else if (c == 4) {
				internalFormat = (gammaCorrection) ? GL_SRGB_ALPHA : GL_RGBA; // GL_SRGB_ALPHA => OpenGL will correct image to linear color space
				dataFormat = GL_RGBA;
			}

			if (!textureID) {
				width = w;
				height = h;
				nrComponents = c;
				glGenTextures(1, &textureID);
				glBindTexture(GL_TEXTURE_2D_ARRAY, textureID);

				glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, internalFormat, width, height, paths.size(), 0, dataFormat, GL_UNSIGNED_SHORT, NULL);
				glTexImage3D(GL_TEXTURE_2D_ARRAY, 1, internalFormat, width / 2, height / 2, paths.size(), 0, dataFormat, GL_UNSIGNED_SHORT, NULL);
				glTexImage3D(GL_TEXTURE_2D_ARRAY, 2, internalFormat, width / 4, height / 4, paths.size(), 0, dataFormat, GL_UNSIGNED_SHORT, NULL);
				glTexImage3D(GL_TEXTURE_2D_ARRAY, 3, internalFormat, width / 8, height / 8, paths.size(), 0, dataFormat, GL_UNSIGNED_SHORT, NULL);
			}
			if (width != w || height != h || nrComponents != c) {
				std::cout << "Texture don't have same format!" << path << std::endl;
				stbi_image_free(data);
				return -1;
			}
			glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, i, width, height, 1, dataFormat, GL_UNSIGNED_SHORT, data);
			i++;
			stbi_image_free(data);
		}
		else
		{
			std::cout << "Texture Array failed to load at path: " << path << std::endl;
			stbi_image_free(data);
			return -1;
		}
	}

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glGenerateMipmap(GL_TEXTURE_2D_ARRAY);
	glBindTexture(GL_TEXTURE_2D_ARRAY, 0);

	return textureID;
}