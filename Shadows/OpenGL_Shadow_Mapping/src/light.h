#ifndef LIGHT_H
#define LIGHT_H

#include <glad\glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "shader_m.h"
#include <string>

#include <vector>

enum Lighttypes {DIRECTIONLIGHT, POINTLIGHT, SPOTLIGHT};

// Default Light values
const glm::vec3 POSITION(1.0f, 1.0f, 1.0f);
const glm::vec3 DIRECTION(0.0f, 0.0f, 0.0f);
const glm::vec3 COLOR(1.0f, 1.0f, 1.0f);
const float CONSTANT = 1.0f;
const float LINEAR = 0.09f;
const float QUADRATIC = 0.032f;
const float CUTOFF = glm::cos(glm::radians(12.5f));
const float OUTERCTOFF = glm::cos(glm::radians(17.5f));
const int ID = 1;
const Lighttypes TYPE = POINTLIGHT;

// set up vertex data (and buffer(s)) and configure vertex attributes
// Position (x,y,z) | Color (r,g,b) 
// ------------------------------------------------------------------
const float vertices[] = {
	//first & second 
	-0.5f, -0.5f, -0.5f, // 1.0f, 0.0f, 0.0f,
	0.5f, -0.5f, -0.5f, // 1.0f, 0.0f, 0.0f,
	0.5f,  0.5f, -0.5f, // 1.0f, 0.0f, 0.0f,
	-0.5f,  0.5f, -0.5f, // 1.0f, 0.0f, 0.0f,
	//third & forth
	-0.5f, -0.5f,  0.5f, //  0.0f, 1.0f, 0.0f,
	0.5f, -0.5f,  0.5f, // 0.0f, 1.0f, 0.0f,
	0.5f,  0.5f,  0.5f, // 0.0f, 1.0f, 0.0f,
	-0.5f,  0.5f,  0.5f, // 0.0f, 1.0f, 0.0f,
	//fifth & sixth
	-0.5f,  0.5f,  0.5f, // 0.0f, 0.0f, 1.0f,
	-0.5f,  0.5f, -0.5f, // 0.0f, 0.0f, 1.0f,
	-0.5f, -0.5f, -0.5f, // 0.0f, 0.0f, 1.0f,
	-0.5f, -0.5f,  0.5f, // 0.0f, 0.0f, 1.0f,
	//seventh & eights
	0.5f,  0.5f,  0.5f, // 1.0f, 1.0f, 0.0f,
	0.5f,  0.5f, -0.5f, // 1.0f, 1.0f, 0.0f,
	0.5f, -0.5f, -0.5f, // 1.0f, 1.0f, 0.0f,
	0.5f, -0.5f,  0.5f, // 1.0f, 1.0f, 0.0f,
	//nineth & tenth
	-0.5f, -0.5f, -0.5f, // 1.0f, 1.0f, 1.0f,
	0.5f, -0.5f, -0.5f, // 1.0f, 1.0f, 1.0f,
	0.5f, -0.5f,  0.5f, // 1.0f, 1.0f, 1.0f,
	-0.5f, -0.5f,  0.5f, // 1.0f, 1.0f, 1.0f,
	//eleventh & twelves
	-0.5f,  0.5f, -0.5f, // 0.0f, 1.0f, 1.0f,
	0.5f,  0.5f, -0.5f, // 0.0f, 1.0f, 1.0f,
	0.5f,  0.5f,  0.5f, // 0.0f, 1.0f, 1.0f,
	-0.5f,  0.5f,  0.5f, // 0.0f, 1.0f, 1.0f,
};

unsigned int indices[] = {
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

//Light 
class Light
{
public:
	glm::vec3 position;
	glm::vec3 ambient;
	glm::vec3 diffuse;
	glm::vec3 specular;
	glm::vec3 direction;

	float constant;
	float linear;
	float quadratic;
	float cutOff;
	float outerCutOff;
	bool used = false;
	int id;
	Lighttypes type;

	unsigned int VBO, VAO, EBO;
	unsigned int lightVAO;

	//Constructor with vectors
	Light(int i = ID,
		  glm::vec3 pos = POSITION,
		  glm::vec3 dir = DIRECTION,
		  glm::vec3 color = COLOR,
		  
		  float con = CONSTANT, 
		  float lin = LINEAR, 
		  float quad = QUADRATIC,
		  float cut = CUTOFF,
		  float oCut = OUTERCTOFF, 
		  Lighttypes typ= TYPE)
	{
		id = i;
		position = pos;
		direction = dir;
		ambient = color;
		diffuse = color;
		specular = color;
		constant = con;
		linear = lin;
		quadratic = quad;
		cutOff = cut;
		outerCutOff = oCut;
		type = typ;
	}

	//Constructor with vectors
	Light(int i = ID, 
		  glm::vec3 pos = POSITION,
		  glm::vec3 dir = DIRECTION,
		  glm::vec3 amb = COLOR, 
		  glm::vec3 diff = COLOR,
		  glm::vec3 spec = COLOR,
		  float con = CONSTANT,
		  float lin = LINEAR,
		  float quad = QUADRATIC,
		  float cut = CUTOFF,
		  float oCut = OUTERCTOFF,
		  Lighttypes typ= TYPE)
	{
		id = i;
		position = pos;
		direction = dir;
		ambient = amb;
		diffuse = diff;
		specular = spec;
		constant = con;
		linear = lin;
		quadratic = quad;
		cutOff = cut;
		outerCutOff = oCut;
		type = typ;
	}

	//Constructor with scalars
	Light(float x, float y, float z, float r, float g, float b,
		Lighttypes typ= TYPE, int i = ID)
	{
		id = i;
		position = glm::vec3(x,y,z);
		ambient = glm::vec3(r, g, b);
		diffuse = glm::vec3(r, g, b);
		specular = glm::vec3(r, g, b);
		constant = CONSTANT;
		linear = LINEAR;
		quadratic = QUADRATIC;
		cutOff = CUTOFF;
		outerCutOff = OUTERCTOFF;
		type = typ;
	}

	void Draw(Shader shader) {
			glBindVertexArray(lightVAO);
			glm::mat4 model = glm::mat4(1.0f);
			model = glm::translate(model, position);
			model = glm::scale(model, glm::vec3(0.2f));
			shader.setMat4("model", model);
			shader.setVec3("lightColor.ambient", ambient);
			shader.setVec3("lightColor.diffuse", diffuse);
			shader.setVec3("lightColor.specular", specular);
			//draw Lamp Object
			glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);

	}

	void initialise() {
		
		glGenVertexArrays(1, &VAO);
		glGenBuffers(1, &VBO);
		glGenBuffers(1, &EBO);
		//bind the Vertex Array Object first, the bind and set vertex buffer(s), and then configure vertex attributes(s).
		glBindVertexArray(VAO);

		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

		glGenVertexArrays(1, &lightVAO);
		glBindVertexArray(lightVAO);
		// we only need to bind to the VBO, the container's VBO's data already contains the correct data.
		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		// set the vertex attributes (only position data for our lamp)
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
	}

	//Benutzung des Shaders
	void use(Shader shader) {
		switch (type) {
			used = true;
			case DIRECTIONLIGHT:
				shader.setVec3("dirLight.direction", direction);
				shader.setVec3("dirLight.ambient", ambient);
				shader.setVec3("dirLight.diffuse", diffuse);
				shader.setVec3("dirLight.specular", specular);
				break;
			case POINTLIGHT:
				shader.setVec3(std::string("pointLights[") + std::to_string(id) + std::string("].position"), position);
				shader.setVec3(std::string("pointLights[") + std::to_string(id) + std::string("].ambient"), ambient);
				shader.setVec3(std::string("pointLights[") + std::to_string(id) + std::string("].diffuse"), diffuse);
				shader.setVec3(std::string("pointLights[") + std::to_string(id) + std::string("].specular"), specular);
				shader.setFloat(std::string("pointLights[") + std::to_string(id) + std::string("].constant"), constant);
				shader.setFloat(std::string("pointLights[") + std::to_string(id) + std::string("].linear"), linear);
				shader.setFloat(std::string("pointLights[") + std::to_string(id) + std::string("].quadratic"), quadratic);
				break;
			case SPOTLIGHT:
				shader.setVec3("spotLight.position", position);
				shader.setVec3("spotLight.direction", direction);
				shader.setVec3("spotLight.ambient", ambient);
				shader.setVec3("spotLight.diffuse", diffuse);
				shader.setVec3("spotLight.specular", specular);
				shader.setFloat("spotLight.constant", constant);
				shader.setFloat("spotLight.linear", linear);
				shader.setFloat("spotLight.quadratic", quadratic);
				shader.setFloat("spotLight.cutOff", cutOff);
				shader.setFloat("spotLight.outerCutOff", outerCutOff);
				break;
		}
	}
private:
	

};
#endif