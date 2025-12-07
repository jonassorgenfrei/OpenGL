/* 
	SRC-CODE FOR 
	- DEPTH, STENCILE BUFFER & Framebuffers + Blending, 
	- Face Culling
	- cube map (skybox & environment mapping)
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
unsigned int loadTexture(const char *path, GLenum wrap);

// settings
const unsigned int SCR_WIDTH = 1280;
const unsigned int SCR_HEIGHT = 720;

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

	// enable influencing the point size in the vertex shader
	glEnable(GL_PROGRAM_POINT_SIZE);

	// build and compile our shader program
	// ------------------------------------
	//Shader shaderVariables(FileSystem::getSamplePath("shader/shaderVariables.vert").c_str(), FileSystem::getSamplePath("shader/shaderVariables.frag").c_str());
	Shader shaderRed(FileSystem::getSamplePath("shader/advancedglsl.vert").c_str(), FileSystem::getSamplePath("shader/red.frag").c_str());
	Shader shaderGreen(FileSystem::getSamplePath("shader/advancedglsl.vert").c_str(), FileSystem::getSamplePath("shader/green.frag").c_str());
	Shader shaderBlue(FileSystem::getSamplePath("shader/advancedglsl.vert").c_str(), FileSystem::getSamplePath("shader/blue.frag").c_str());
	Shader shaderYellow(FileSystem::getSamplePath("shader/advancedglsl.vert").c_str(), FileSystem::getSamplePath("shader/yellow.frag").c_str());

	// set up vertex data (and buffer(s)) and configure vertex attributes
	// ------------------------------------------------------------------
	float cubeVertices[] = {
		// Back face
		-0.5f, -0.5f, -0.5f,  0.0f, 0.0f, // Bottom-left
		0.5f,  0.5f, -0.5f,  1.0f, 1.0f,// top-right
		0.5f, -0.5f, -0.5f,  1.0f, 0.0f,// bottom-right         
		0.5f,  0.5f, -0.5f,  1.0f, 1.0f,// top-right
		-0.5f, -0.5f, -0.5f,  0.0f, 0.0f,// bottom-left
		-0.5f,  0.5f, -0.5f,  0.0f, 1.0f,// top-left
		// Front face
		-0.5f, -0.5f,  0.5f,  0.0f, 0.0f,// bottom-left
		0.5f, -0.5f,  0.5f,  1.0f, 0.0f,// bottom-right
		0.5f,  0.5f,  0.5f,  1.0f, 1.0f,// top-right
		0.5f,  0.5f,  0.5f,  1.0f, 1.0f,// top-right
		-0.5f,  0.5f,  0.5f,  0.0f, 1.0f,// top-left
		-0.5f, -0.5f,  0.5f,  0.0f, 0.0f,// bottom-left
		// Left face
		-0.5f,  0.5f,  0.5f,  1.0f, 0.0f,// top-right
		-0.5f,  0.5f, -0.5f,  1.0f, 1.0f,// top-left
		-0.5f, -0.5f, -0.5f,  0.0f, 1.0f,// bottom-left
		-0.5f, -0.5f, -0.5f,  0.0f, 1.0f,// bottom-left
		-0.5f, -0.5f,  0.5f,  0.0f, 0.0f,// bottom-right
		-0.5f,  0.5f,  0.5f,  1.0f, 0.0f,// top-right
		// Right face
		0.5f,  0.5f,  0.5f,  1.0f, 0.0f,// top-left
		0.5f, -0.5f, -0.5f,  0.0f, 1.0f,// bottom-right
		0.5f,  0.5f, -0.5f,  1.0f, 1.0f,// top-right         
		0.5f, -0.5f, -0.5f,  0.0f, 1.0f,// bottom-right
		0.5f,  0.5f,  0.5f,  1.0f, 0.0f,// top-left
		0.5f, -0.5f,  0.5f,  0.0f, 0.0f,// bottom-left     
		// Bottom face
		-0.5f, -0.5f, -0.5f,  0.0f, 1.0f,// top-right
		0.5f, -0.5f, -0.5f,  1.0f, 1.0f,// top-left
		0.5f, -0.5f,  0.5f,  1.0f, 0.0f,// bottom-left
		0.5f, -0.5f,  0.5f,  1.0f, 0.0f,// bottom-left
		-0.5f, -0.5f,  0.5f,  0.0f, 0.0f,// bottom-right
		-0.5f, -0.5f, -0.5f,  0.0f, 1.0f,// top-right
		// Top face
		-0.5f,  0.5f, -0.5f,  0.0f, 1.0f,// top-left
		0.5f,  0.5f,  0.5f,  1.0f, 0.0f,// bottom-right
		0.5f,  0.5f, -0.5f,  1.0f, 1.0f,// top-right     
		0.5f,  0.5f,  0.5f,  1.0f, 0.0f,// bottom-right
		-0.5f,  0.5f, -0.5f,  0.0f, 1.0f,// top-left
		-0.5f,  0.5f,  0.5f,  0.0f, 0.0f // bottom-left
	};
	// cube VAO
	unsigned int cubeVAO, cubeVBO;
	glGenVertexArrays(1, &cubeVAO);
	glGenBuffers(1, &cubeVBO);
	glBindVertexArray(cubeVAO);
	glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), &cubeVertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));

	/* ------------ Advanced Data ------------ */
	/** 
	 *	fill specific regions of the buffer 
	 *	Parameters: 
	 *	1.	buffertarget
	 *	2.	offset
	 *	3.	size of data
	 *	4.	actual Data
	 *
	 *	=> allows: to insert/update only vertain parts of the buffer's memory
	 *	- buffer should have enough allocated memory --> glBufferData() before!
	 */
	 //	glBufferSubData(GL_ARRAY_BUFFER, 24, sizeof(data), &data); // Range: [24, 24 + sizeof(data)]

	/**
	 *	-> ask for a pointer to the buffer's memory and directly copy the data to the buffer
	 *	useful to directly map data to a buffer, without first storing it in temporary memory
	 */
	// float data[] = { 0.5f, 1.0f, -0.35f,... };
	// glBindBuffer(GL_ARRAY_BUFFER, buffer);
		/* get pointer */
	// void *ptr = glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
		/* now copy data into memory */
	// memcpy(ptr, data, sizeof(data));
		/* make sure to tell OpenGL we're done with the pointer, pointer becomes invalid and the function
			returns GL_TRUE if OpenGL was able to map your data successfully to the buffer */
	// glUnmapBuffer(GL_ARRAY_BUFFER);

	/** 
	 * Batching vertex attributes 
	 * batch all vector data into large chunks per attribute type 
	 * (e.g. 123123123123 instead of: 111122223333)
	 */
	//	float position[] = { ... };
	//	float normals[] = { ... };
	//	float tex[] = { ... };
		/* fill buffer (transfer the attribute arrays as a whole into the buffer without 
						first having to process them) 
		 */
	//	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(positions), &positions);
	//	glBufferSubData(GL_ARRAY_BUFFER, sizeof(positions), sizeof(normals), &normals);
	//	glBufferSubData(GL_ARRAY_BUFFER, sizeof(positions) + sizeof(normals), sizeof(tex), &tex);
		/* update the vertex attribute pointers to reflect these changes 
			=> stride parameter is equal to the size of the vertex attribute
		*/
	//	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), 0);
	//	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*) (sizeof(positions)));
	//	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*) (sizeof(positions) + sizeof(normals)));
		
	/**
	 * Copying Buffers
	 * readtarget & writetarget: buffer targets that we want to copy from & to
	 * for copying there are 2 more buffer targets (GL_COPY_READ_BUFFER & GL_COPY_WRITE_BUFFER)
	 */
	//	void glCopyBufferSubData(GLenum readtarget, GLenum writetarget, GLintptr readoffset, GLintptr writeoffset, GLsizeiptr size);
	/* example of copying the content of 2 vertex array buffers: */
	//	float vertexData[] = { ... };
		/* bind buffers to COPY Buffers */
	//	glBindBuffer(GL_COPY_READ_BUFFER, vbo1);
	//	glBindBuffer(GL_COPY_WRITE_BUFFER, vbo2);
		/* set COPY Buffers as readtarget & writetarget argument 
			set size, copy from read data with readoffset to writetarget at writeoffset */
	//	glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, 0, 0, sizeof(vertexData));
	/* alternative: */
	//	float vertexData[] = { ... };
	//	glBindBuffer(GL_ARRAY_BUFFER, vbo1);
	//	glBindBuffer(GL_COPY_WRITE_BUFFER, vbo2);
	//	glCopyBufferData(GL_ARRAY_BUFFER, GL_COPY_WRITE_BUFFER, 0, 0, sizeof(vertexData));


	 // configure a uniform buffer object
	// ---------------------------------
	// first. We get the relevant block indices
	unsigned int uniformBlockIndexRed = glGetUniformBlockIndex(shaderRed.ID, "Matrices");
	unsigned int uniformBlockIndexGreen = glGetUniformBlockIndex(shaderGreen.ID, "Matrices");
	unsigned int uniformBlockIndexBlue = glGetUniformBlockIndex(shaderBlue.ID, "Matrices");
	unsigned int uniformBlockIndexYellow = glGetUniformBlockIndex(shaderYellow.ID, "Matrices");
	// then we link each shader's uniform block to this uniform binding point
	glUniformBlockBinding(shaderRed.ID, uniformBlockIndexRed, 0);
	glUniformBlockBinding(shaderGreen.ID, uniformBlockIndexGreen, 0);
	glUniformBlockBinding(shaderBlue.ID, uniformBlockIndexBlue, 0);
	glUniformBlockBinding(shaderYellow.ID, uniformBlockIndexYellow, 0);
	// Now actually create the buffer
	unsigned int uboMatrices;
	glGenBuffers(1, &uboMatrices);
	glBindBuffer(GL_UNIFORM_BUFFER, uboMatrices);
	glBufferData(GL_UNIFORM_BUFFER, 2 * sizeof(glm::mat4), NULL, GL_STATIC_DRAW);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
	// define the range of the buffer that links to a uniform binding point
	glBindBufferRange(GL_UNIFORM_BUFFER, 0, uboMatrices, 0, 2 * sizeof(glm::mat4));

	// store the projection matrix (we only do this once now) (note: we're not using zoom anymore by changing the FoV)
	glm::mat4 projection = glm::perspective(45.0f, (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
	glBindBuffer(GL_UNIFORM_BUFFER, uboMatrices);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(glm::mat4), glm::value_ptr(projection));
	glBindBuffer(GL_UNIFORM_BUFFER, 0);


	// load and create a texture 
	// -------------------------
	/*unsigned int frontTexture = loadTexture(FileSystem::getPath("content/images/container.jpg").c_str(), GL_REPEAT);
	unsigned int backTexture = loadTexture(FileSystem::getPath("content/images/wall.jpg").c_str(), GL_REPEAT);*/

	// tell opengl for each sampler to which texture unit it belongs to (only has to be done once)
	// -------------------------------------------------------------------------------------------
	
	// shader configuration
	// --------------------
	/*glm::mat4 projection = glm::perspective(45.0f, (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
	shaderVariables.use();
	shaderVariables.setMat4("projection", projection);

	shaderVariables.setInt("frontTexture", 0); // or with shader class
	shaderVariables.setInt("backTexture", 1); // or with shader class */

	/* Using uniform buffers */
	/*unsigned int uboMatrices;
	glGenBuffers(1, &uboMatrices);
	glBindBuffer(GL_UNIFORM_BUFFER, uboMatrices);
	glBufferData(GL_UNIFORM_BUFFER, 2 * sizeof(glm::mat4), NULL, GL_STATIC_DRAW); // allocate 150 bytes of memory
	glBindBuffer(GL_UNIFORM_BUFFER, 0);*/
	//whenever update or insert data into buffer (memory) -> bind to uboMatrices & use glBufferSubData 


	// TODO: HERE AND AT DIAGRAMM SHADER A B @ https://learnopengl.com/Advanced-OpenGL/Advanced-GLSL


	// draw as wireframe
	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	
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
		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); 

		/*glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, frontTexture);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, backTexture);*/

		// set the view and projection matrix in the uniform block - we only have to do this once per loop iteration.
		glm::mat4 view = camera.GetViewMatrix();
		glBindBuffer(GL_UNIFORM_BUFFER, uboMatrices);
		glBufferSubData(GL_UNIFORM_BUFFER, sizeof(glm::mat4), sizeof(glm::mat4), glm::value_ptr(view));
		glBindBuffer(GL_UNIFORM_BUFFER, 0);

		// draw 4 cubes 
		// RED
		glBindVertexArray(cubeVAO);
		shaderRed.use();
		glm::mat4 model = glm::mat4(1);
		model = glm::translate(model, glm::vec3(-0.75f, 0.75f, 0.0f)); // move top-left
		shaderRed.setMat4("model", model);
		glDrawArrays(GL_TRIANGLES, 0, 36);
		// GREEN
		shaderGreen.use();
		model = glm::mat4(1);
		model = glm::translate(model, glm::vec3(0.75f, 0.75f, 0.0f)); // move top-right
		shaderGreen.setMat4("model", model);
		glDrawArrays(GL_TRIANGLES, 0, 36);
		// YELLOW
		shaderYellow.use();
		model = glm::mat4(1);
		model = glm::translate(model, glm::vec3(-0.75f, -0.75f, 0.0f)); // move bottom-left
		shaderYellow.setMat4("model", model);
		glDrawArrays(GL_TRIANGLES, 0, 36);
		// BLUE
		shaderBlue.use();
		model = glm::mat4(1);
		model = glm::translate(model, glm::vec3(0.75f, -0.75f, 0.0f)); // move bottom-right
		shaderBlue.setMat4("model", model);
		glDrawArrays(GL_TRIANGLES, 0, 36);


		/*glBindVertexArray(cubeVAO);
		shaderVariables.use();

		glm::mat4 model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(-0.75f, 0.75f, 0.0f)); // move top-left
		shaderVariables.setMat4("view", view);
		shaderVariables.setMat4("model", model);
		glDrawArrays(GL_TRIANGLES, 0, 36);*/





		// glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
		// ------------------------------------------------------------------------------_
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	// optional: de-allocate all resources once they've outlived their purpose:
	// ------------------------------------------------------------------------
	glDeleteVertexArrays(1, &cubeVAO);
	glDeleteBuffers(1, &cubeVBO);
	
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