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