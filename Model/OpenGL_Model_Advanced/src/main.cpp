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
	GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Model Advanced", NULL, NULL);
	
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
	/* Face Culling */
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glFrontFace(GL_CCW);

	// build and compile our shader program
	// ------------------------------------
	Shader lightShader(FileSystem::getPath("shader/light.vs").c_str(), FileSystem::getPath("shader/light.fs").c_str());
	Shader modelShader(FileSystem::getPath("shader/modelShader.vs").c_str(), FileSystem::getPath("shader/modelShader.fs").c_str());
	Shader skyboxShader(FileSystem::getPath("shader/skyShader.vs").c_str(), FileSystem::getPath("shader/skyShader.fs").c_str());

	// load models 
	// -----------
	Model ourModel(FileSystem::getPath("models/nanosuit/nanosuit.obj").c_str());

	// set up vertex data (and buffer(s)) and configure vertex attributes
	// Position (x,y,z) | Color (r,g,b) | Texturcoordinate (x,y) | Normal (x,y,z)
	// ------------------------------------------------------------------
	float vertices[] = {
		//first 
		-0.5f, -0.5f, -0.5f,  1.0f, 0.0f, 0.0f,  0.0f, 0.0f,  0.0f,  0.0f, -1.0f, //0
		0.5f, -0.5f, -0.5f,  1.0f, 0.0f, 0.0f,  1.0f, 0.0f,  0.0f,  0.0f, -1.0f, //1
		0.5f,  0.5f, -0.5f,  1.0f, 0.0f, 0.0f,  1.0f, 1.0f,  0.0f,  0.0f, -1.0f, //2
		//second
		// 2 0.5f,  0.5f, -0.5f,  1.0f, 0.0f, 0.0f,  1.0f, 1.0f,  0.0f,  0.0f, -1.0f,
		-0.5f,  0.5f, -0.5f,  1.0f, 0.0f, 0.0f,  0.0f, 1.0f,  0.0f,  0.0f, -1.0f, //3
		// 0 -0.5f, -0.5f, -0.5f,  1.0f, 0.0f, 0.0f,  0.0f, 0.0f,  0.0f,  0.0f, -1.0f,

		//third
		-0.5f, -0.5f,  0.5f,  0.0f, 1.0f, 0.0f,  0.0f, 0.0f,  0.0f,  0.0f, 1.0f, //4
		0.5f, -0.5f,  0.5f,  0.0f, 1.0f, 0.0f,  1.0f, 0.0f,  0.0f,  0.0f, 1.0f, //5
		0.5f,  0.5f,  0.5f,  0.0f, 1.0f, 0.0f,  1.0f, 1.0f,  0.0f,  0.0f, 1.0f, //6
		//forth
		// 6 0.5f,  0.5f,  0.5f,  0.0f, 1.0f, 0.0f,  1.0f, 1.0f,  0.0f,  0.0f, 1.0f,
		-0.5f,  0.5f,  0.5f,  0.0f, 1.0f, 0.0f,  0.0f, 1.0f,  0.0f,  0.0f, 1.0f, //7
		// 4 -0.5f, -0.5f,  0.5f,  0.0f, 1.0f, 0.0f,  0.0f, 0.0f,  0.0f,  0.0f, 1.0f,

		//fifth
		-0.5f,  0.5f,  0.5f,  0.0f, 0.0f, 1.0f,  1.0f, 0.0f, -1.0f,  0.0f,  0.0f, //8
		-0.5f,  0.5f, -0.5f,  0.0f, 0.0f, 1.0f,  1.0f, 1.0f, -1.0f,  0.0f,  0.0f, //9
		-0.5f, -0.5f, -0.5f,  0.0f, 0.0f, 1.0f,  0.0f, 1.0f, -1.0f,  0.0f,  0.0f, //10
		//sixth
		// 10 -0.5f, -0.5f, -0.5f,  0.0f, 1.0f, 0.0f,  0.0f, 1.0f, -1.0f,  0.0f,  0.0f,
		-0.5f, -0.5f,  0.5f,  0.0f, 0.0f, 1.0f,  0.0f, 0.0f, -1.0f,  0.0f,  0.0f, //11
		// 8 -0.5f,  0.5f,  0.5f,  0.0f, 1.0f, 0.0f,  1.0f, 0.0f, -1.0f,  0.0f,  0.0f,

		//seventh
		0.5f,  0.5f,  0.5f,  1.0f, 1.0f, 0.0f,  1.0f, 0.0f,  1.0f,  0.0f,  0.0f, //12
		0.5f,  0.5f, -0.5f,  1.0f, 1.0f, 0.0f,  1.0f, 1.0f,  1.0f,  0.0f,  0.0f, //13
		0.5f, -0.5f, -0.5f,  1.0f, 1.0f, 0.0f,  0.0f, 1.0f,  1.0f,  0.0f,  0.0f, //14
		//eights
		// 14 0.5f, -0.5f, -0.5f,  1.0f, 1.0f, 0.0f,  0.0f, 1.0f,  1.0f,  0.0f,  0.0f, 
		0.5f, -0.5f,  0.5f,  1.0f, 1.0f, 0.0f,  0.0f, 0.0f,  1.0f,  0.0f,  0.0f, //15
		// 12 0.5f,  0.5f,  0.5f,  1.0f, 1.0f, 0.0f,  1.0f, 0.0f,  1.0f,  0.0f,  0.0f, 

		//nineth
		-0.5f, -0.5f, -0.5f,  1.0f, 1.0f, 1.0f,  0.0f, 1.0f,  0.0f, -1.0f,  0.0f, //16
		0.5f, -0.5f, -0.5f,  1.0f, 1.0f, 1.0f,  1.0f, 1.0f,  0.0f, -1.0f,  0.0f, //17
		0.5f, -0.5f,  0.5f,  1.0f, 1.0f, 1.0f,  1.0f, 0.0f,  0.0f, -1.0f,  0.0f, //18
		//tenth
		// 18 0.5f, -0.5f,  0.5f,  1.0f, 1.0f, 1.0f,  1.0f, 0.0f,  0.0f, -1.0f,  0.0f,
		-0.5f, -0.5f,  0.5f,  1.0f, 1.0f, 1.0f,  0.0f, 0.0f,  0.0f, -1.0f,  0.0f, //19
		// 16 -0.5f, -0.5f, -0.5f,  1.0f, 1.0f, 1.0f,  0.0f, 1.0f,  0.0f, -1.0f,  0.0f,

		//eleventh
		-0.5f,  0.5f, -0.5f,  0.0f, 1.0f, 1.0f,  0.0f, 1.0f,  0.0f,  1.0f,  0.0f, //20
		0.5f,  0.5f, -0.5f,  0.0f, 1.0f, 1.0f,  1.0f, 1.0f,  0.0f,  1.0f,  0.0f, //21
		0.5f,  0.5f,  0.5f,  0.0f, 1.0f, 1.0f,  1.0f, 0.0f,  0.0f,  1.0f,  0.0f, //22
		//twelves
		// 22 0.5f,  0.5f,  0.5f,  0.0f, 1.0f, 1.0f,  1.0f, 0.0f,  0.0f,  1.0f,  0.0f,
		-0.5f,  0.5f,  0.5f,  0.0f, 1.0f, 1.0f,  0.0f, 0.0f,  0.0f,  1.0f,  0.0f //23
		// 20 -0.5f,  0.5f, -0.5f,  0.0f, 1.0f, 1.0f,  0.0f, 1.0f,  0.0f,  1.0f,  0.0f
	};

	unsigned int indices[] = {  // note that we start from 0!
		0, 1, 2,    // first triangle
		2, 3, 0,    // second triangle

		4, 5, 6,	// third triangle
		6, 7, 4,    // forth triangle

		8, 9, 10,    // fifth triangle
		10, 11, 8,	// sixth triangle

		12, 13, 14,    // seventh triangle
		14, 15, 12,    // eights triangle

		16, 17, 18,	// nineth triangle
		18, 19, 16,    // tenth triangle

		20, 21, 22,    // eleventh triangle
		22, 23, 20,	// twelves triangle
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

	//positions of the point lights
	glm::vec3 pointLightPositions[] = {
		glm::vec3(-0.7f,  0.2f,  2.0f),
		glm::vec3(2.3f, -3.3f, -4.0f),
		glm::vec3(-4.0f,  2.0f, -12.0f),
		glm::vec3(0.0f,  0.0f, -3.0f)
	};

	unsigned int VBO, VAO, EBO;
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &EBO);
	//bind the Vertex Array Object first, the bind and set vertex buffer(s), and then configure vertex attributes(s).
	glBindVertexArray(VAO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	// position attribute 
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	// color coord attribute
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);
	// texture coord attribute
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(6 * sizeof(float)));
	glEnableVertexAttribArray(2);
	// normal coord attribute
	glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(8 * sizeof(float)));
	glEnableVertexAttribArray(3);
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

	/* Lamp */
	//directional Light
	Light dirLight = Light(//id
							0,
							//Position
							glm::vec3(0.0f, 0.0f, 0.0f),
							//Direction 
							glm::vec3(-0.2f, -1.0f, -0.3f),
							//Ambient				
							glm::vec3(0.05f, 0.05f, 0.05f),
							//Diffuse
							glm::vec3(0.4f, 0.4f, 0.4f),
							//Specular
							glm::vec3(0.5f, 0.5f, 0.5f),
							//Constant
							1.0f,
							//Linear
							0.09f,
							//Quadratic
							0.032f,
							//cut
							0.0f,
							//ocut
							0.0f,
							//Type
							DIRECTIONLIGHT);
	//Point Lights
	Light pointLights[] = { Light(//id
								0,
								//Position
								pointLightPositions[0],
								//Direction 
								glm::vec3(0.0f, 0.0f, 0.0f),
								//Ambient				
								glm::vec3(0.1f, 0.1f, 0.1f),
								//Diffuse
								glm::vec3(1.0f, 1.0f, 1.0f),
								//Specular
								glm::vec3(1.0f, 1.0f, 1.0f),
								//Constant
								1.0f,
								//Linear
								0.09f,
								//Quadratic
								0.032f,
								//cut
								0.0f,
								//ocut
								0.0f,
								//Type
								POINTLIGHT
								),
						Light(//id
							1,
							//Position
							pointLightPositions[1],
							//Direction 
							glm::vec3(0.0f, 0.0f, 0.0f),
							//Ambient				
							glm::vec3(0.0f, 0.05f, 0.0f),
							//Diffuse
							glm::vec3(0.0f, 0.8f, 0.0f),
							//Specular
							glm::vec3(0.0f, 1.0f, 0.0f),
							//Constant
							1.0f,
							//Linear
							0.09f,
							//Quadratic
							0.032f,
							//cut
							0.0f,
							//ocut
							0.0f,
							//Type
							POINTLIGHT
						),
						Light(//id
							2,
							//Position
							pointLightPositions[2],
							//Direction 
							glm::vec3(0.0f, 0.0f, 0.0f),
							//Ambient				
							glm::vec3(0.05f, 0.05f, 0.05f),
							//Diffuse
							glm::vec3(0.8f, 0.8f, 0.8f),
							//Specular
							glm::vec3(1.0f, 1.0f, 1.0f),
							//Constant
							1.0f,
							//Linear
							0.09f,
							//Quadratic
							0.032f,
							//cut
							0.0f,
							//ocut
							0.0f,
							//Type
							POINTLIGHT
						),
						Light(//id
							3,
							//Position
							pointLightPositions[3],
							//Direction 
							glm::vec3(0.0f, 0.0f, 0.0f),
							//Ambient				
							glm::vec3(0.05f, 0.0f, 0.0f),
							//Diffuse
							glm::vec3(0.8f, 0.0f, 0.0f),
							//Specular
							glm::vec3(1.0f, 0.0f, 0.0f),
							//Constant
							1.0f,
							//Linear
							0.09f,
							//Quadratic
							0.032f,
							//cut
							0.0f,
							//ocut
							0.0f,
							//Type
							POINTLIGHT
						),
		
						 };
	Light spotLight = Light(//id
							0,
							//Position
							camera.Position,
							//Direction 
							camera.focus,
							//Ambient				
							glm::vec3(0.1f, 0.1f, 0.1f),
							//Diffuse
							glm::vec3(0.9f, 0.9f, 0.9f),
							//Specular
							glm::vec3(1.0f, 1.0f, 1.0f),
							//Constant
							1.0f,
							//Linear
							0.09f,
							//Quadratic
							0.032f,
							//cut
							glm::cos(glm::radians(12.5f)),
							//ocut
							glm::cos(glm::radians(15.0f)),
							//Type
							SPOTLIGHT);

	for (int i = 0; i < pointLightsCount; i++) {
		pointLights[i].initialise();
	}

	// load and create a texture 
	// -------------------------

	// cubmap 
	vector<std::string> faces
	{
		FileSystem::getPath("images/skybox/right.jpg").c_str(),
		FileSystem::getPath("images/skybox/left.jpg").c_str(),
		FileSystem::getPath("images/skybox/top.jpg").c_str(),
		FileSystem::getPath("images/skybox/bottom.jpg").c_str(),
		FileSystem::getPath("images/skybox/front.jpg").c_str(),
		FileSystem::getPath("images/skybox/back.jpg").c_str(),
	};

	unsigned int cubemapTexture = loadCubemap(faces, GL_CLAMP_TO_EDGE);

	// tell opengl for each sampler to which texture unit it belongs to (only has to be done once)
	// -------------------------------------------------------------------------------------------
	
	modelShader.use();
	modelShader.setInt("skybox", 4);

	skyboxShader.use();
	skyboxShader.setInt("skybox", 0);


	// render loop
	// -----------
	while (!glfwWindowShouldClose(window))
	{
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
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // also clear the depth buffer now!
		glEnable(GL_DEPTH_TEST);
		// bind textures on corresponding texture units
		//light.position = glm::vec3(2*sin(glfwGetTime()), 0.0f, 2*cos(glfwGetTime()));
		//light.Position.x = 1.0f + sin(glfwGetTime()) * 2.0f;
		//light.Position.y = sin(glfwGetTime() / 2.0f) * 1.0f;
		spotLight.position = camera.Position;
		spotLight.direction = camera.Front;
		
		/*
		glm::vec3 lightColor;
		lightColor.x = sin(glfwGetTime() * 2.0f);
		lightColor.y = sin(glfwGetTime() * 0.7f);
		lightColor.z = sin(glfwGetTime() * 1.3f);

		light.diffuse = lightColor * glm::vec3(0.5f);
		light.ambient = lightColor * glm::vec3(0.2f);
		*/

		modelShader.use();
		dirLight.use(modelShader);
		for (int i = 0; i < pointLightsCount; i++)
			pointLights[i].use(modelShader);
		spotLight.use(modelShader);

		modelShader.setVec3("viewPos", camera.Position);
		//model draw 
		glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
		// 0.1 near clipping plane
		// 100.0 far clipping plane
		glm::mat4 view = camera.GetViewMatrix();
		modelShader.setVec3("cameraPos", camera.Position);
		modelShader.setMat4("projection", projection);
		modelShader.setMat4("view", view);

		glActiveTexture(GL_TEXTURE4);
		glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);

		// render the loaded model
		glm::mat4 model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(0.0f, -1.75f, 0.0f)); // translate it down so it's at the center of the scene
		model = glm::scale(model, glm::vec3(0.2f, 0.2f, 0.2f));	// it's a bit too big for our scene, so scale it down
		modelShader.setMat4("model", model);
		ourModel.Draw(modelShader);
		
		//lighting
		lightShader.use();
		// set the model, view and projection matrix uniforms
		lightShader.setMat4("projection", projection);
		lightShader.setMat4("view", view);
		
		for (int i = 0; i < pointLightsCount; i++) {
			pointLights[i].Draw(lightShader);
		}

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

		// glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
		// ------------------------------------------------------------------------------_
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	// optional: de-allocate all resources once they've outlived their purpose:
	// ------------------------------------------------------------------------
	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);
	glDeleteBuffers(1, &EBO);
	
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

	float cameraSpeed = 2.5f * deltaTime; // adjust accordingly

	if (glfwGetKey(window, GLFW_KEY_F2) == GLFW_PRESS)
	{
		const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
		glfwSetWindowMonitor(window, glfwGetPrimaryMonitor(), 0, 0, mode->width, mode->height, mode->refreshRate);
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
		"...1111..2222...",
		"......1..2......",
		"......1..2222...",
		"...1..1.....2...",
		"...1111..2222...",
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
			else if (logo[y][x] == '2')
				memcpy(target, icon_colors[3], 4);
			else
				memset(target, 0, 4);
			target += 4;
		}
	}

	glfwSetWindowIcon(window, 1, &img);
}