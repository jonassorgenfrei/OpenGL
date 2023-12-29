/* 
	SRC-CODE FOR 
	- DEPTH BUFFER & Framebuffers + Blending, 
	- Face Culling
	- cube map (skybox & environment mapping)
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
unsigned int loadTexture(const char *path, GLenum wrap);
/* load Cubemap */
unsigned int loadCubemap(vector<std::string> textures_faces, GLenum wrap);

void icon(GLFWwindow* window);

// settings
const unsigned int SCR_WIDTH = 1280;
const unsigned int SCR_HEIGHT = 720;
//PointLights
const int pointLightsCount = 4;
//show p-lights
bool showLights = false;
// camera
Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
bool firstMouse = true;
float lastX = SCR_WIDTH / 2.0;
float lastY = SCR_HEIGHT / 2.0;

bool w_pressed = false;
bool wireframe = false;

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
	// Depth test function
	glDepthFunc(GL_LESS);
	/* 
	Function 	Description
	GL_ALWAYS 	The depth test always passes.
	GL_NEVER 	The depth test never passes.
	GL_LESS 	Passes if the fragment's depth value is less than the stored depth value.
	GL_EQUAL 	Passes if the fragment's depth value is equal to the stored depth value.
	GL_LEQUAL 	Passes if the fragment's depth value is less than or equal to the stored depth value.
	GL_GREATER 	Passes if the fragment's depth value is greater than the stored depth value.
	GL_NOTEQUAL Passes if the fragment's depth value is not equal to the stored depth value.
	GL_GEQUAL 	Passes if the fragment's depth value is greater than or equal to the stored depth value.
	*/

	//Blending
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	/*
		Parameters
		1) source factor
		2) destination factor
			
			GL_ZERO						Factor equal to 0
			GL_ONE						Factor equal to 1
			GL_SRC_COLOR				Factor equal to the src color vector
			GL_ONE_MINUS_SRC_COLOR		Factor equal to 1 minus src color vector
			GL_DST_COLOR				Factor equal to the destination color vector
			GL_ONE_MINUS_DST_COLOR		Factor equal to 1 minus destination color vector
			GL_SRC_ALPHA				Factor equal to alpha component of src color vector
			GL_ONE_MINUS_SRC_ALPHA		Factor equal to 1 minus alpha of src color vector
			GL_DST_ALPHA				Factor equal to alpha component of dest color vector
			GL_ONE_MINUS_DST_ALPHA		Factor equal to 1 minus alpha of the dest color vector
			GL_CONSTANT_COLOR			Factor equal to constant color vector
			GL_ONE_MINUS_CONSTANT_COLOR	Factor equal to 1 minus constant color vector
			GL_CONSTANT_ALPHA			Factor equal to alpha component of constant color vector
			GL_ONE_MINUS_CONSTANT_ALPHA	Factor equal to 1 minus alpha of constant color vector
	*/
	// glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ZERO);
	/* sets the RGB components as set previous, but only lets the resulting alpha component be 
	 influenced by the src alpha value */
	// glBlendEquation(GLenum mode)
	/* set operation for Blending
		GL_FUNC_ADD					adds both components to each other	res = src + dest
		GL_FUNC_SUBTRACT			substracts both components from each other	res = src - dst
		GL_FUNC_REVERSE_SUBSTRACT	substracts both components, but reverse order res = dst - src
		*/

	/* Face Culling */
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	/*
		GL_BACK				Culls only the back faces
		GL_FRONT			Culls only the front faces
		GL_FRON_AND_BACK	Culls both, the front and the back faces
	*/
	glFrontFace(GL_CCW);
	/*	
		GL_CCW		count-clockwise
		GL_CW		clockwise
		*/

	/* Framebuffer */
	unsigned int fbo;
	glGenFramebuffers(1, &fbo);
	/* bind Framebuffer
		glBindFramebuffer(GL_FRAMEBUFFER, fbo);
		targe all the next read and write frambuffer operations will affect current bound frambuffer

		complete Framebuffer:
		-> attach at least one buffer (color, depth, or stencil)
		-> at least one color attachment
		-> all attachments should be complete as well (reserved memory
		-> each buffer should have the same number of samples
	*/
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE)
	{	// execute vitory dance
		glBindFramebuffer(GL_FRAMEBUFFER, fbo);
		//make sure all rendering operations will have a visual impact on the main window
	}
	//when done with all framebuffer operations, delete fbo
	//glDeleteFramebuffers(1, &fbo);

	// Texture attachments
	unsigned int texture;
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
	//dimension equal to screen size
	// NULL as texture's data parameter (only allocating memory and not acutally filling it
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glBindTexture(GL_TEXTURE_2D, 0);

	// if rendering to a texture smaller/larger size => call glViewport (before rendering)
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);
	// attach texture to framebuffer
	/*
		Parameters:
		target			framebuffer type => targeting (draw, read or both)
		attachment		type of attachment; color attachament
				GL_COLOR_ATTACHMENT									Color texture
				GL_DEPTH_ATTACHMENT			GL_DEPTH_COMPONENT		Depth texture
				GL_STENCIL_ATTACHMENT		GL_STENCIL_INDEX		Stencil texture
		texarget		type of texture to attach
		texture			actual texture to attach
		level			mipmap level
	*/

	/* attach depth & stencil buffer as single texture
			glTexImage2D(
			  GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, 800, 600, 0, 
			  GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, NULL
			);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, texture, 0);
			each 32bit value of text consists for 24 Bits depth, 8 bit stencil information
	
	*/

	/* Renderbuffer object attachments */
	/* array of byte, integers, pixels .... ; generally write-only, possible to read from via glReadPixels, 
	returns specifiv area of pixels from currently bound framebuffer */
	//creating renderbuffer object
	unsigned int rbo;
	glGenRenderbuffers(1, &rbo);
	glBindRenderbuffer(GL_RENDERBUFFER, rbo);
	//creating depth and stencil render buffer obj. 
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, SCR_WIDTH, SCR_HEIGHT);
	//similar to texture obj; but specifically designed to be used as an image instead of general pupose data buffer (like texture)
	// GL_DEPTH24_STENCIL8 => internal format (holds depth & stencil buffer, with 24 & 8 bit)

	glBindRenderbuffer(GL_RENDERBUFFER, 0);
	// once enough memory for the renderbuffer is allocated => unbind renderbuffer

	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);
	//actually attach renderbuffer obj

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	//unbind frameuffer to make sure not accidentally rendering to the wrong fb

	// build and compile our shader program
	// ------------------------------------
	Shader shader(FileSystem::getPath("shader/shader.vs").c_str(), FileSystem::getPath("shader/shader.fs").c_str());
	Shader shaderSingleColor(FileSystem::getPath("shader/shader.vs").c_str(), FileSystem::getPath("shader/shaderSingleColor.fs").c_str());
	Shader screenShader(FileSystem::getPath("shader/framebufferShader.vs").c_str(), FileSystem::getPath("shader/framebufferShader.fs").c_str());
	Shader skyboxShader(FileSystem::getPath("shader/skyShader.vs").c_str(), FileSystem::getPath("shader/skyShader.fs").c_str());
	Shader reflectionShader(FileSystem::getPath("shader/reflection.vs").c_str(), FileSystem::getPath("shader/reflection.fs").c_str());
	Shader refractionShader(FileSystem::getPath("shader/refraction.vs").c_str(), FileSystem::getPath("shader/refraction.fs").c_str());

	// set up vertex data (and buffer(s)) and configure vertex attributes
	// ------------------------------------------------------------------
	float cubeVertices[] = {
		// Back face
		-0.5f, -0.5f, -0.5f,  0.0f, 0.0f,  0.0f,  0.0f, -1.0f, // Bottom-left
		0.5f,  0.5f, -0.5f,  1.0f, 1.0f,  0.0f,  0.0f, -1.0f, // top-right
		0.5f, -0.5f, -0.5f,  1.0f, 0.0f,  0.0f,  0.0f, -1.0f, // bottom-right         
		0.5f,  0.5f, -0.5f,  1.0f, 1.0f,  0.0f,  0.0f, -1.0f, // top-right
		-0.5f, -0.5f, -0.5f,  0.0f, 0.0f,  0.0f,  0.0f, -1.0f, // bottom-left
		-0.5f,  0.5f, -0.5f,  0.0f, 1.0f,  0.0f,  0.0f, -1.0f, // top-left
		// Front face
		-0.5f, -0.5f,  0.5f,  0.0f, 0.0f,  0.0f,  0.0f, 1.0f, // bottom-left
		0.5f, -0.5f,  0.5f,  1.0f, 0.0f,  0.0f,  0.0f, 1.0f, // bottom-right
		0.5f,  0.5f,  0.5f,  1.0f, 1.0f,  0.0f,  0.0f, 1.0f, // top-right
		0.5f,  0.5f,  0.5f,  1.0f, 1.0f,  0.0f,  0.0f, 1.0f, // top-right
		-0.5f,  0.5f,  0.5f,  0.0f, 1.0f,  0.0f,  0.0f, 1.0f, // top-left
		-0.5f, -0.5f,  0.5f,  0.0f, 0.0f,  0.0f,  0.0f, 1.0f, // bottom-left
		// Left face
		-0.5f,  0.5f,  0.5f,  1.0f, 0.0f, -1.0f,  0.0f,  0.0f, // top-right
		-0.5f,  0.5f, -0.5f,  1.0f, 1.0f, -1.0f,  0.0f,  0.0f, // top-left
		-0.5f, -0.5f, -0.5f,  0.0f, 1.0f, -1.0f,  0.0f,  0.0f, // bottom-left
		-0.5f, -0.5f, -0.5f,  0.0f, 1.0f, -1.0f,  0.0f,  0.0f, // bottom-left
		-0.5f, -0.5f,  0.5f,  0.0f, 0.0f, -1.0f,  0.0f,  0.0f, // bottom-right
		-0.5f,  0.5f,  0.5f,  1.0f, 0.0f, -1.0f,  0.0f,  0.0f, // top-right
		// Right face
		0.5f,  0.5f,  0.5f,  1.0f, 0.0f,  1.0f,  0.0f,  0.0f, // top-left
		0.5f, -0.5f, -0.5f,  0.0f, 1.0f,  1.0f,  0.0f,  0.0f, // bottom-right
		0.5f,  0.5f, -0.5f,  1.0f, 1.0f,  1.0f,  0.0f,  0.0f, // top-right         
		0.5f, -0.5f, -0.5f,  0.0f, 1.0f,  1.0f,  0.0f,  0.0f, // bottom-right
		0.5f,  0.5f,  0.5f,  1.0f, 0.0f,  1.0f,  0.0f,  0.0f, // top-left
		0.5f, -0.5f,  0.5f,  0.0f, 0.0f,  1.0f,  0.0f,  0.0f, // bottom-left     
		// Bottom face
		-0.5f, -0.5f, -0.5f,  0.0f, 1.0f,  0.0f, -1.0f,  0.0f, // top-right
		0.5f, -0.5f, -0.5f,  1.0f, 1.0f,  0.0f, -1.0f,  0.0f, // top-left
		0.5f, -0.5f,  0.5f,  1.0f, 0.0f,  0.0f, -1.0f,  0.0f, // bottom-left
		0.5f, -0.5f,  0.5f,  1.0f, 0.0f,  0.0f, -1.0f,  0.0f, // bottom-left
		-0.5f, -0.5f,  0.5f,  0.0f, 0.0f,  0.0f, -1.0f,  0.0f, // bottom-right
		-0.5f, -0.5f, -0.5f,  0.0f, 1.0f,  0.0f, -1.0f,  0.0f, // top-right
		// Top face
		-0.5f,  0.5f, -0.5f,  0.0f, 1.0f,  0.0f,  1.0f,  0.0f, // top-left
		0.5f,  0.5f,  0.5f,  1.0f, 0.0f,  0.0f,  1.0f,  0.0f, // bottom-right
		0.5f,  0.5f, -0.5f,  1.0f, 1.0f,  0.0f,  1.0f,  0.0f, // top-right     
		0.5f,  0.5f,  0.5f,  1.0f, 0.0f,  0.0f,  1.0f,  0.0f, // bottom-right
		-0.5f,  0.5f, -0.5f,  0.0f, 1.0f,  0.0f,  1.0f,  0.0f, // top-left
		-0.5f,  0.5f,  0.5f,  0.0f, 0.0f,  0.0f,  1.0f,  0.0f  // bottom-left   
	};
	float planeVertices[] = {
		// positions          // texture Coords (note we set these higher than 1 (together with GL_REPEAT as texture wrapping mode). this will cause the floor texture to repeat)
		5.0f, -0.5f,  5.0f,  2.0f, 0.0f,
		-5.0f, -0.5f, -5.0f,  0.0f, 2.0f,
		-5.0f, -0.5f,  5.0f,  0.0f, 0.0f,

		5.0f, -0.5f,  5.0f,  2.0f, 0.0f,
		5.0f, -0.5f, -5.0f,  2.0f, 2.0f,
		-5.0f, -0.5f, -5.0f,  0.0f, 2.0f
	};
	float transparentVertices[] = {
		// positions         // texture Coords (swapped y coordinates because texture is flipped upside down)
		0.0f,  0.5f,  0.0f,  0.0f,  0.0f,
		0.0f, -0.5f,  0.0f,  0.0f,  1.0f,
		1.0f, -0.5f,  0.0f,  1.0f,  1.0f,

		0.0f,  0.5f,  0.0f,  0.0f,  0.0f,
		1.0f, -0.5f,  0.0f,  1.0f,  1.0f,
		1.0f,  0.5f,  0.0f,  1.0f,  0.0f
	};
	float quadVertices[] = { // vertex attributes for a quad that fills the entire screen in Normalized Device Coordinates.
		// positions   // texCoords
		-1.0f,  1.0f,  0.0f, 1.0f,
		-1.0f, -1.0f,  0.0f, 0.0f,
		1.0f, -1.0f,  1.0f, 0.0f,

		-1.0f,  1.0f,  0.0f, 1.0f,
		1.0f, -1.0f,  1.0f, 0.0f,
		1.0f,  1.0f,  1.0f, 1.0f
	};

	float skyboxVertices[] = {
		//positions
		-1.0f,  1.0f, -1.0f,
		-1.0f, -1.0f, -1.0f,
		1.0f, -1.0f, -1.0f,
		1.0f, -1.0f, -1.0f,
		1.0f,  1.0f, -1.0f,
		-1.0f,  1.0f, -1.0f,

		-1.0f, -1.0f,  1.0f,
		-1.0f, -1.0f, -1.0f,
		-1.0f,  1.0f, -1.0f,
		-1.0f,  1.0f, -1.0f,
		-1.0f,  1.0f,  1.0f,
		-1.0f, -1.0f,  1.0f,

		1.0f, -1.0f, -1.0f,
		1.0f, -1.0f,  1.0f,
		1.0f,  1.0f,  1.0f,
		1.0f,  1.0f,  1.0f,
		1.0f,  1.0f, -1.0f,
		1.0f, -1.0f, -1.0f,

		-1.0f, -1.0f,  1.0f,
		-1.0f,  1.0f,  1.0f,
		1.0f,  1.0f,  1.0f,
		1.0f,  1.0f,  1.0f,
		1.0f, -1.0f,  1.0f,
		-1.0f, -1.0f,  1.0f,

		-1.0f,  1.0f, -1.0f,
		1.0f,  1.0f, -1.0f,
		1.0f,  1.0f,  1.0f,
		1.0f,  1.0f,  1.0f,
		-1.0f,  1.0f,  1.0f,
		-1.0f,  1.0f, -1.0f,

		-1.0f, -1.0f, -1.0f,
		-1.0f, -1.0f,  1.0f,
		1.0f, -1.0f, -1.0f,
		1.0f, -1.0f, -1.0f,
		-1.0f, -1.0f,  1.0f,
		1.0f, -1.0f,  1.0f
	};

	// cube VAO
	unsigned int cubeVAO, cubeVBO;
	glGenVertexArrays(1, &cubeVAO);
	glGenBuffers(1, &cubeVBO);
	glBindVertexArray(cubeVAO);
	glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), &cubeVertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(5 * sizeof(float)));
	glBindVertexArray(0);
	// plane VAO
	unsigned int planeVAO, planeVBO;
	glGenVertexArrays(1, &planeVAO);
	glGenBuffers(1, &planeVBO);
	glBindVertexArray(planeVAO);
	glBindBuffer(GL_ARRAY_BUFFER, planeVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(planeVertices), &planeVertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
	glBindVertexArray(0);
	// transparent VAO
	unsigned int transparentVAO, transparentVBO;
	glGenVertexArrays(1, &transparentVAO);
	glGenBuffers(1, &transparentVBO);
	glBindVertexArray(transparentVAO);
	glBindBuffer(GL_ARRAY_BUFFER, transparentVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(transparentVertices), transparentVertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
	glBindVertexArray(0);
	//quad
	unsigned int quadVAO, quadVBO;
	glGenVertexArrays(1, &quadVAO);
	glGenBuffers(1, &quadVBO);
	glBindVertexArray(quadVAO);
	glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
	glBindVertexArray(0);
	// sky VAO
	unsigned int skyVAO, skyVBO;
	glGenVertexArrays(1, &skyVAO);
	glGenBuffers(1, &skyVBO);
	glBindVertexArray(skyVAO);
	glBindBuffer(GL_ARRAY_BUFFER, skyVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glBindVertexArray(0);


	// grass 
	vector<glm::vec3> vegetation;
	vegetation.push_back(glm::vec3(-1.5f, 0.0f, 0.48f));
	vegetation.push_back(glm::vec3(1.5f, 0.0f, 0.51f));
	vegetation.push_back(glm::vec3(0.0f, 0.0f, 0.7f));
	vegetation.push_back(glm::vec3(-0.3f, 0.0f, -2.3f));
	vegetation.push_back(glm::vec3(0.5f, 0.0f, -0.6f));

	// load and create a texture 
	// -------------------------
	unsigned int cubeTexture = loadTexture(FileSystem::getPath("../../content/images/container.jpg").c_str(), GL_REPEAT);
	unsigned int floorTexture = loadTexture(FileSystem::getPath("../../content/images/metal.png").c_str(), GL_REPEAT);
	// repeat, interpolates the border values with the next repeated value
	unsigned int grassTexture = loadTexture(FileSystem::getPath("../../content/images/grass.png").c_str(), GL_CLAMP_TO_EDGE);
	// set Clamp to edge whenever using alpha textures, 
	unsigned int transparentTexture = loadTexture(FileSystem::getPath("../../content/images/blending_transparent_window.png").c_str(), GL_CLAMP_TO_EDGE);

	// cubmap 
	vector<std::string> faces
	{
		FileSystem::getPath("../../content/images/skybox/right.jpg").c_str(),
		FileSystem::getPath("../../content/images/skybox/left.jpg").c_str(),
		FileSystem::getPath("../../content/images/skybox/top.jpg").c_str(),
		FileSystem::getPath("../../content/images/skybox/bottom.jpg").c_str(),
		FileSystem::getPath("../../content/images/skybox/front.jpg").c_str(),
		FileSystem::getPath("../../content/images/skybox/back.jpg").c_str(),
	};

	unsigned int cubemapTexture = loadCubemap(faces, GL_CLAMP_TO_EDGE);

	// tell opengl for each sampler to which texture unit it belongs to (only has to be done once)
	// -------------------------------------------------------------------------------------------
	
	// shader configuration
	// --------------------
	shader.use();
	shader.setInt("texture1", 0);

	skyboxShader.use();
	skyboxShader.setInt("skybox", 0);

	reflectionShader.use();
	reflectionShader.setInt("skybox", 0);

	refractionShader.use();
	refractionShader.setInt("skybox", 0);

	// draw as wireframe
	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	
	// render loop
	// -----------
	while (!glfwWindowShouldClose(window))
	{
		std::map<float, glm::vec3> sorted;
		for (unsigned int i = 0; i < vegetation.size(); i++)
		{
			float distance = glm::length(camera.Position - vegetation[i]);
			//sorted
			sorted[distance] = vegetation[i];

			//unsorted 
			//sorted[float(i)] = vegetation[i];
		}

		// per-frame time logic
		// --------------------
		float currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		// input
		// -----
		processInput(window);

		// render
		// ------
		// first pass
		glBindFramebuffer(GL_FRAMEBUFFER, fbo);
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT); 
		// also clear the depth buffer & stencile buffer now!
		glEnable(GL_DEPTH_TEST);
		//reenable depth test

		// draw full/wireframe
		glPolygonMode(GL_FRONT_AND_BACK, wireframe ? GL_LINE : GL_FILL);

		// set uniforms
		//single Color Shader
		shaderSingleColor.use();
		glm::mat4 model = glm::mat4(1.0f);
		glm::mat4 view = camera.GetViewMatrix();
		glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
		shaderSingleColor.setMat4("view", view);
		shaderSingleColor.setMat4("projection", projection);
		// normal shader
		shader.use();
		shader.setMat4("view", view);
		shader.setMat4("projection", projection);
		// reflection Shader
		reflectionShader.use();
		reflectionShader.setMat4("view", view);
		reflectionShader.setMat4("projection", projection);
		reflectionShader.setVec3("cameraPos", camera.Position);
		// refration Shader
		refractionShader.use();
		refractionShader.setMat4("view", view);
		refractionShader.setMat4("projection", projection);
		refractionShader.setVec3("cameraPos", camera.Position);

		shader.use();
		// floor
		glBindVertexArray(planeVAO);
		glBindTexture(GL_TEXTURE_2D, floorTexture);
		shader.setMat4("model", glm::mat4(1.0f));
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glBindVertexArray(0);
		
		// 1st. render pass, draw objects as normal
		// --------------------------------------------------------------------
		// cubes
		glBindVertexArray(cubeVAO);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, cubeTexture);
		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(-1.0f, 0.0f, -1.0f));
		shader.setMat4("model", model);
		glDrawArrays(GL_TRIANGLES, 0, 36);
		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(2.0f, 0.0f, 0.0f));
		shader.setMat4("model", model);
		glDrawArrays(GL_TRIANGLES, 0, 36);
		
		/* Reflection Cube*/
		reflectionShader.use();
		glBindVertexArray(cubeVAO);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
		//glBindTexture(GL_TEXTURE_2D, cubeTexture);
		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(-1.0f, 2.0f, -1.0f));
		reflectionShader.setMat4("model", model);
		glDrawArrays(GL_TRIANGLES, 0, 36);

		/* Refraction Cube */
		refractionShader.use();
		glBindVertexArray(cubeVAO);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
		//glBindTexture(GL_TEXTURE_2D, cubeTexture);
		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(2.0f, 2.0f, 0.0f));
		refractionShader.setMat4("model", model);
		glDrawArrays(GL_TRIANGLES, 0, 36);
		
		//Skybox
		/* render first & disable depth writing, => always be at the background of all the other objs. */
		glDepthFunc(GL_LEQUAL);  // change depth function so depth test passes when values are equal to depth buffer's content
		skyboxShader.use();
		// set view and projection matrix
		skyboxShader.setMat4("view", glm::mat4(glm::mat3(camera.GetViewMatrix())));
		skyboxShader.setMat4("projection", projection);
		glBindVertexArray(skyVAO);
		glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
		glDrawArrays(GL_TRIANGLES, 0, 36);
		glBindVertexArray(0);
		glDepthFunc(GL_LESS); // set depth function back to default

		/* Transparency */
		shader.use();

		//Windows
		glBindVertexArray(transparentVAO);
		glBindTexture(GL_TEXTURE_2D, transparentTexture);

		glDisable(GL_CULL_FACE);
		for (std::map<float, glm::vec3>::reverse_iterator it = sorted.rbegin(); it != sorted.rend(); ++it)
		{
			for (int p = 0; p < 360; p += 360) {
				// each of the grass objects => single Quad
				model = glm::mat4(1.0f);
				model = glm::translate(model, it->second);
				model = glm::translate(model, glm::vec3(0.5f, 0.0f, 0.0f));
				model = glm::rotate(model, (float)p, glm::vec3(0.0f, 1.0f, 0.0f));
				model = glm::translate(model, glm::vec3(-0.5f, 0.0f, 0.0f));
				shader.setMat4("model", model);
				glDrawArrays(GL_TRIANGLES, 0, 6);
			}
		}	
		glEnable(GL_CULL_FACE);

		glBindFramebuffer(GL_FRAMEBUFFER, 0); // back to default
		glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		// draw full/wireframe
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

		screenShader.use();
		glBindVertexArray(quadVAO);
		glDisable(GL_DEPTH_TEST);
		glBindTexture(GL_TEXTURE_2D, texture);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		

		// glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
		// ------------------------------------------------------------------------------_
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	// optional: de-allocate all resources once they've outlived their purpose:
	// ------------------------------------------------------------------------
	glDeleteVertexArrays(1, &cubeVAO);
	glDeleteVertexArrays(1, &planeVAO);
	glDeleteVertexArrays(1, &skyVAO);
	glDeleteBuffers(1, &cubeVBO);
	glDeleteBuffers(1, &planeVBO);
	glDeleteBuffers(1, &skyVBO);
	
	// glfw: terminate, clearing all previously allocated GLFW resources.
	// ------------------------------------------------------------------
	glfwTerminate();
	return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window)
{	
	if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS)
		showLights = !showLights;

	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);

	if (glfwGetKey(window, GLFW_KEY_F2) == GLFW_PRESS)
	{
		const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
		glfwSetWindowMonitor(window, glfwGetPrimaryMonitor(), 0, 0, mode->width, mode->height, mode->refreshRate);
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
// utility function for loading a cube Map texture
// -----------------------------------------------
unsigned int loadCubemap(vector<std::string> textures_faces, GLenum wrap)
{
	unsigned int textureID;
	glGenTextures(1, &textureID);

	glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);
	int width, height, nrChannels;
	for (GLuint i = 0; i < textures_faces.size(); i++)
	{
		unsigned char *data = stbi_load(textures_faces[i].c_str(), &width, &height, &nrChannels, 0);
		if (data)
		{
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
		}
		else {
			std::cout << "Cubemap texture failed to load at path: " << textures_faces[i] << std::endl;
		}
		stbi_image_free(data);
		
	}
	/*
		GL_TEXTURE_CUBE_MAP_POSITIVE_X	RIGHT
		GL_TEXTURE_CUBE_MAP_NEGATIVE_X	LEFT
		GL_TEXTURE_CUBE_MAP_POSITIVE_Y	TOP
		GL_TEXTURE_CUBE_MAP_NEGATIVE_Y	BOTTOM
		GL_TEXTURE_CUBE_MAP_POSITIVE_Z	BACK
		GL_TEXTURE_CUBE_MAP_NEGATIVE_Z	FRONT
	*/

	/* Filtering & warpping methods */
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	// R for the 3rd dimension (like the z for positions)

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