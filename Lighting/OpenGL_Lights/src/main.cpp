#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "shader_m.h"
#include "stb_image.h"
#include "camera.h"

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
unsigned int loadTexture(const char *path);

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;
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
	GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
	//Vollbild
	
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

	// configure global opengl state
	// -----------------------------
	glEnable(GL_DEPTH_TEST);

	// build and compile our shader program
	// ------------------------------------
	Shader ourShader(FileSystem::getPath("shader/cube.vs").c_str(), FileSystem::getPath("shader/cube.fs").c_str());
	Shader lightShader(FileSystem::getPath("shader/light.vs").c_str(), FileSystem::getPath("shader/light.fs").c_str());
	
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

	// world space positions of our cubes
	glm::vec3 cubePositions[] = {
		glm::vec3(0.0f,  0.0f,  0.0f),
		glm::vec3(2.0f,  5.0f, -15.0f),
		glm::vec3(-1.5f, -2.2f, -2.5f),
		glm::vec3(-3.8f, -2.0f, -12.3f),
		glm::vec3(2.4f, -0.4f, -3.5f),
		glm::vec3(-1.7f,  3.0f, -7.5f),
		glm::vec3(1.3f, -2.0f, -2.5f),
		glm::vec3(1.5f,  2.0f, -2.5f),
		glm::vec3(1.5f,  0.2f, -1.5f),
		glm::vec3(-1.3f,  1.0f, -1.5f)
	};

	//positions of the point lights
	glm::vec3 pointLightPositions[] = {
		glm::vec3(0.7f,  0.2f,  2.0f),
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
	
	Material esmerald = Material(76.8f, 
									glm::vec3(0.0215f, 0.1745f, 0.0215f),
									glm::vec3(0.07568f, 0.61424f, 0.07568f),
									glm::vec3(0.633f, 0.727811f, 0.633f)
								);

	Material pearl = Material(11.264f,
									glm::vec3(0.25f, 0.20725f, 0.20725f),
									glm::vec3(1.0f, 0.829f, 0.829f),
									glm::vec3(0.296648f, 0.296648f, 0.296648f)
								);

	Material bronze = Material(25.6f,
									glm::vec3(0.2125f, 0.1275f, 0.054f),
									glm::vec3(0.714f, 0.4284f, 0.18144f),
									glm::vec3(0.393548f, 0.271906f, 0.166721f)
								);

	Material gold = Material(51.2,
									glm::vec3(0.24725f, 0.1995f, 0.0745f),
									glm::vec3(0.75164f, 0.60648f, 0.22648f),
									glm::vec3(0.628281f, 0.555802f, 0.366065f)
								);

	Material materials[4] = {};
	materials[0] = esmerald;
	materials[1] = pearl;
	materials[2] = bronze;
	materials[3] = gold;

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
		
						 };
	Light spotLight = Light(//id
							0,
							//Position
							camera.Position,
							//Direction 
							camera.focus,
							//Ambient				
							glm::vec3(0.0f, 0.0f, 0.0f),
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
							glm::cos(glm::radians(12.5f)),
							//ocut
							glm::cos(glm::radians(15.0f)),
							//Type
							SPOTLIGHT);

	unsigned int lightVAO;
	glGenVertexArrays(1, &lightVAO);
	glBindVertexArray(lightVAO);
	// we only need to bind to the VBO, the container's VBO's data already contains the correct data.
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	// set the vertex attributes (only position data for our lamp)
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)0);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);


	// load and create a texture 
	// -------------------------

	//unsigned int container = loadTexture(FileSystem::getPath("../../content/images/container.png").c_str());
	unsigned int diffuseMap = loadTexture(FileSystem::getPath("../../content/images/container2.png").c_str());
	unsigned int specularMap = loadTexture(FileSystem::getPath("../../content/images/container2_specular.png").c_str());
	unsigned int emissionMap= loadTexture(FileSystem::getPath("../../content/images/smoke.jpg").c_str());
	//unsigned int face = loadTexture(FileSystem::getPath("../../content/images/awesomeface.png").c_str());

	// tell opengl for each sampler to which texture unit it belongs to (only has to be done once)
	// -------------------------------------------------------------------------------------------
	
	ourShader.use();
	//ourShader.setInt("texture1", container);
	//ourShader.setInt("texture2", face);
	ourShader.setInt("material.diffuse", 0);
	ourShader.setInt("material.specular", 1);
	ourShader.setInt("material.emission", 2);

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

		// bind textures on corresponding texture units
		/*glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, container);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, face);*/

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

		// activate shader 
		ourShader.use();
		dirLight.use(ourShader);
		for (int i = 0; i < pointLightsCount; i++)
			pointLights[i].use(ourShader);
		spotLight.use(ourShader);
		ourShader.setVec3("viewPos", camera.Position);

		// pass projection matrix to shader (note that in this cas it could change every frame)
		glm::mat4 projection = glm::mat4(1.0f);
		projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
		ourShader.setMat4("projection", projection);

		// camera/view transformation 
		glm::mat4 view = glm::mat4(1.0f);
		view = camera.GetViewMatrix();
		ourShader.setMat4("view", view);

		// bind diffuse map
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, diffuseMap);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, specularMap);
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, emissionMap);

		//render boxes
		glBindVertexArray(VAO);
		for (unsigned int i = 0; i < 10; i++)
		{
			//calculate the model matrix for each object and pass it to shader before drawing
			glm::mat4 model = glm::mat4(1.0f);
			model = glm::translate(model, cubePositions[i]);
			float angle = 20.0f * (i+1) * glfwGetTime();
			model = glm::rotate(model, glm::radians(angle), glm::vec3(1.0f, 0.3f, 0.5f));
			ourShader.setMat4("model", model);
			// Set Material
			materials[i % 4].use(ourShader);
			glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
		}

		if (showLights) {
			//lighting
			lightShader.use();
			// set the model, view and projection matrix uniforms
			lightShader.setMat4("projection", projection);
			lightShader.setMat4("view", view);

			//we now draw as many light bulbs as we have point lights.
			glBindVertexArray(lightVAO);
			for (unsigned int i = 0; i < pointLightsCount; i++)
			{
				glm::mat4 model = glm::mat4(1.0f);
				model = glm::translate(model, pointLights[i].position);
				model = glm::scale(model, glm::vec3(0.2f));
				lightShader.setMat4("model", model);
				//draw Lamp Object
				glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
			}
		}
		

		// glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
		// ------------------------------------------------------------------------------_
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	// optional: de-allocate all resources once they've outlived their purpose:
	// ------------------------------------------------------------------------
	glDeleteVertexArrays(1, &VAO);
	glDeleteVertexArrays(1, &lightVAO);
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
unsigned int loadTexture(char const * path)
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

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
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