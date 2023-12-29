#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "shader_s.h"
#include "stb_image.h"
#include "filesystem.h"

#include <iostream>

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow *window);

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

float mixValue = 0.0f;

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
	//const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
	//glfwSetWindowMonitor(window, glfwGetPrimaryMonitor(), 0, 0, mode->width, mode->height, mode->refreshRate);
	if (window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

	// glad: load all OpenGL function pointers
	// ---------------------------------------
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
		return -1;
	}

	// build and compile our shader program
	// ------------------------------------
	Shader ourShader(FileSystem::getPath("shader/shader.vs").c_str(), FileSystem::getPath("shader/shaderfs1.fs").c_str()); 
	Shader ourShader2(FileSystem::getPath("shader/shader.vs").c_str(), FileSystem::getPath("shader/shaderfs3.fs").c_str()); 
	Shader ourShader3(FileSystem::getPath("shader/shader.vs").c_str(), FileSystem::getPath("shader/shaderfs2.fs").c_str());
	Shader textureShader(FileSystem::getPath("shader/textureShader.vs").c_str(), FileSystem::getPath("shader/textureShader.fs").c_str());
	Shader transformShader(FileSystem::getPath("shader/transform.vs").c_str(), FileSystem::getPath("shader/transform.fs").c_str());
	
	// set up vertex data (and buffer(s)) and configure vertex attributes
	// ------------------------------------------------------------------
	float firstTriangle[] = {
		// positions			// colors
		-1.0f, -1.0f, 0.0f,		1.0f, 0.0f, 0.0f,	// left 
		1.0f, -1.0f, 0.0f,		0.0f, 1.0f, 0.0f,	// right
		0.0f, 1.0f, 0.0f,		0.0f, 0.0f, 1.0f	// top 
	};
	float secondTriangle[] = {
		// positions			// colors
		0.0f, -0.5f, 0.0f,  1.0f, 0.0f, 0.0f,	// left
		0.9f, -0.5f, 0.0f,  0.0f, 1.0f, 0.0f,	// right
		0.45f, 0.5f, 0.0f,  0.0f, 0.0f, 1.0f	// top 
	};
	float vertices[] = {
		// positions          // colors           // texture coords
		0.45f,  0.5f, 0.0f,   1.0f, 0.0f, 0.0f,   1.0f, 1.0f,   // top right   
		0.45f, -0.5f, 0.0f,  0.0f, 1.0f, 0.0f,   1.0f, 0.0f,   // bottom right
		-0.45f, -0.5f, 0.0f,  0.0f, 0.0f, 1.0f,   0.0f, 0.0f,   // bottom left
		-0.45f,  0.5f, 0.0f,  1.0f, 1.0f, 0.0f,   0.0f, 1.0f    // top left 
	};
	unsigned int indices[] = {  // note that we start from 0!
		0, 1, 3,   // first triangle
		1, 2, 3    // second triangle
	};
	//Quader 
	float quader[] = {
		// positions          // colors           // texture coords
		0.5f,  0.5f, 0.0f,   1.0f, 0.0f, 0.0f,   1.0f, 1.0f,   // top right
		0.5f, -0.5f, 0.0f,   0.0f, 1.0f, 0.0f,   1.f, 0.0f,   // bottom right
		-0.5f, -0.5f, 0.0f,   0.0f, 0.0f, 1.0f,   0.0f, 0.0f,   // bottom left
		-0.5f,  0.5f, 0.0f,   1.0f, 1.0f, 0.0f,   0.0f, 1.0f    // top left 
	};
	unsigned int quaderIndices[] = {
		0, 1, 3, // first triangle
		1, 2, 3  // second triangle
	};

	unsigned int VBOs[4], VAOs[4], EBOs[2];
	glGenVertexArrays(4, VAOs); // we can also generate multiple VAOs or buffers at the same time
	glGenBuffers(4, VBOs);
	glGenBuffers(2, EBOs);

	// first triangle setup
	// --------------------
	glBindVertexArray(VAOs[0]);
	glBindBuffer(GL_ARRAY_BUFFER, VBOs[0]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(firstTriangle), firstTriangle, GL_STATIC_DRAW);
	// position attribute
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	// color attribute
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);

	// second triangle setup
	// ---------------------
	glBindVertexArray(VAOs[1]);	// note that we bind to a different VAO now
	glBindBuffer(GL_ARRAY_BUFFER, VBOs[1]);	// and a different VBO
	glBufferData(GL_ARRAY_BUFFER, sizeof(secondTriangle), secondTriangle, GL_STATIC_DRAW);
	// position attribute
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	// color attribute
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);

	//EBO TRIANGLES
	// 1. bind Vertex Array Object
	glBindVertexArray(VAOs[2]);
	// 2. copy our vertices array in a vertex buffer for OpenGL to use
	glBindBuffer(GL_ARRAY_BUFFER, VBOs[2]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	// 3. copy our index array in a element buffer for OpenGL to use
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBOs[0]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
	// 4. then set the vertex attributes pointers
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);
	// texture coord attribute
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
	glEnableVertexAttribArray(2);

	//Quader 
	glBindVertexArray(VAOs[3]);

	glBindBuffer(GL_ARRAY_BUFFER, VBOs[3]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quader), quader, GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBOs[1]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(quaderIndices), quaderIndices, GL_STATIC_DRAW);
	// position attribute
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	// color attribute
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);
	// texture coord attribute
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
	glEnableVertexAttribArray(2);

	// load and create a texture 
	// -------------------------
	unsigned int texture1, texture2;
	// texture 1
	//----------
	glGenTextures(1, &texture1);
	glBindTexture(GL_TEXTURE_2D, texture1);
	// set the texture wrapping/filtering options (on the currently bound texture object)
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	// load image, create texture and generate mipmaps
	stbi_set_flip_vertically_on_load(true); // tell stb_image.h to flip loaded texture's on the y-axis.
	// The FileSystem::getPath(...) is part of the GitHub repository so we can find files on any IDE/platform; replace it with your own image path.
	//TODO: USE FILE SYSTEM 
	int width, height, nrChannels;
	unsigned char *data = stbi_load(FileSystem::getPath("../../content/images/container.jpg").c_str(), &width, &height, &nrChannels, 0);

	if (data) {
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
		//Arguments
		// 1) specifies the texture target
		// 2) mipmap level for which we want to create a texture for
		// 3) what kind of format we want to store the texture
		// 4&5) sets width & height of the resulting image
		// 6) should always be 0 (legacy stzdd)
		// 7&8) specify the format and datatype of the source image
		// 9) actual image data
		glGenerateMipmap(GL_TEXTURE_2D);
		// automatically generate all the required mipmaps  for the currently bound texture
	}
	else
	{
		std::cout << "Failed to load texture" << std::endl;
	}
	stbi_image_free(data);
	// texture 2
	//----------
	glGenTextures(1, &texture2);
	glBindTexture(GL_TEXTURE_2D, texture2);
	// set the texture wrapping parameters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	// set texture wrapping to GL_REPEAT (default wrapping method)
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	// set texture filtering parameters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	// load image, create texture and generate mipmaps
	data = stbi_load(FileSystem::getPath("../../content/images/awesomeface.png").c_str(), &width, &height, &nrChannels, 0);

	if (data) {
		// note that the awesomeface.png has transparency and thus an alpha channel, so make sure to tell OpenGL the data type is of GL_RGBA
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		//Arguments
		// 1) specifies the texture target
		// 2) mipmap level for which we want to create a texture for
		// 3) what kind of format we want to store the texture
		// 4&5) sets width & height of the resulting image
		// 6) should always be 0 (legacy stzdd)
		// 7&8) specify the format and datatype of the source image
		// 9) actual image data
		glGenerateMipmap(GL_TEXTURE_2D);
		// automatically generate all the required mipmaps  for the currently bound texture
	}
	else
	{
		std::cout << "Failed to load texture" << std::endl;
	}
	stbi_image_free(data);



	// note that this is allowed, the call to glVertexAttribPointer registered VBO as the vertex attribute's bound vertex buffer object so afterwards we can safely unbind
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	// remember: do NOT unbind the EBO while a VAO is active as the bound element buffer object IS stored in the VAO; keep the EBO bound.
	//glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	// You can unbind the VAO afterwards so other VAO calls won't accidentally modify this VAO, but this rarely happens. Modifying other
	// VAOs requires a call to glBindVertexArray anyways so we generally don't unbind VAOs (nor VBOs) when it's not directly necessary.
	glBindVertexArray(0);

	// uncomment this call to draw in wireframe polygons.
	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	// tell opengl for each sampler to which texture unit it belongs to (only has to be done once)
	// -------------------------------------------------------------------------------------------
	//textureShader.use(); // don't forget to activate/use the shader before setting uniforms!
					 // either set it manually like so:
	//glUniform1i(glGetUniformLocation(textureShader.ID, "texture1"), 0);
	//textureShader.setInt("texture2", 1);	// or set it via the texture class

	transformShader.use();
	transformShader.setInt("texture1", 0);
	transformShader.setInt("texture2", 1);

	// render loop
	// -----------
	while (!glfwWindowShouldClose(window))
	{
		// input
		// -----
		processInput(window);

		// render
		// ------
		glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		// bind Texture
		// bind textures on corresponding texture units

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texture1);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, texture2);

		//create transformations
		glm::mat4 transform = glm::mat4(1.0f);
		//first
		transform = glm::translate(transform, glm::vec3(0.5f, -0.5f, 0.0f));
		transform = glm::rotate(transform, (float)glfwGetTime(), glm::vec3(0.0f, 0.0f, 1.0f));
		transformShader.use();
		// set the texture mix value in the shader
		unsigned int transformLoc = glGetUniformLocation(transformShader.ID, "transform");
		glUniformMatrix4fv(transformLoc, 1, GL_FALSE, glm::value_ptr(transform));
		glBindVertexArray(VAOs[3]);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

		//second
		transform = glm::mat4(1.0f);
		//first
		transform = glm::translate(transform, glm::vec3(-0.5f, 0.5f, 0.0f));
		transform = glm::scale(transform, glm::vec3(sin((float)glfwGetTime()), sin((float)glfwGetTime()), 0.0f));
		transformShader.use();
		// set the texture mix value in the shader
		transformLoc = glGetUniformLocation(transformShader.ID, "transform");
		glUniformMatrix4fv(transformLoc, 1, GL_FALSE, glm::value_ptr(transform));
		glBindVertexArray(VAOs[3]);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

		//THIRD Quader
		//ourShader3.use();
		//glBindVertexArray(VAOs[2]);
		//glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

		// render the triangle
		//ourShader.use();
		//FIRST TRIANGLE
		//glBindVertexArray(VAOs[0]);
		//glDrawArrays(GL_TRIANGLES, 0, 3);	// this call should output an orange triangle

		//ourShader2.use();
		// update the uniform color
		//float timeValue = glfwGetTime();
		//float greenValue = sin(timeValue) / 2.0f + 0.5f;
		//ourShader2.setFloat4("ourColor2", 0.0f, greenValue, 0.0f, 1.0f);
		//SECOND TRIANGLE
		//glBindVertexArray(VAOs[1]);
		//glDrawArrays(GL_TRIANGLES, 0, 3);	// this call should output a yellow triangle
		
		// glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
		// -------------------------------------------------------------------------------
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	// optional: de-allocate all resources once they've outlived their purpose:
	// ------------------------------------------------------------------------
	glDeleteVertexArrays(4, VAOs);
	glDeleteBuffers(4, VBOs);
	glDeleteBuffers(2, EBOs);

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

	if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
	{
		mixValue += 0.001f;
		if (mixValue >= 1.0f)
			mixValue = 1.0f;
	}

	if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
	{
		mixValue -= 0.001f;
		if (mixValue <= 0.0f)
			mixValue = 0.0f;
	}
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	// make sure the viewport matches the new window dimensions; note that width and 
	// height will be significantly larger than specified on retina displays.
	glViewport(0, 0, width, height);
}