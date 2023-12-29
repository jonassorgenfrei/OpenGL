#ifndef MATERIAL_H
#define MATERIAL_H

#include <glad\glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "shader_m.h"

#include <vector>

// Default Material values
const glm::vec3 AMBIENT(1.0f, 1.0f, 1.0f);
const glm::vec3 DIFFUSE(1.0f, 1.0f, 1.0f);
const glm::vec3 SPECULAR(1.0f, 1.0f, 1.0f);

class Material
{
public:
	glm::vec3 ambient;
	glm::vec3 diffuse;
	glm::vec3 specular;
	float shininess;

	//Constructor with vectors
	Material(float shine = 1.0f,
			 glm::vec3 amb = AMBIENT,
			 glm::vec3 diff = DIFFUSE, 
			 glm::vec3 spec = SPECULAR)
	{
		ambient = amb;
		diffuse = diff;
		specular = spec;
		shininess = shine;
	}

	// Benutzung des Shaders
	void use(Shader shader) {
		shader.setVec3("material.ambient", ambient);
		shader.setVec3("material.diffuse", diffuse);
		shader.setVec3("material.specular", specular);
		shader.setFloat("material.shininess",shininess);
	}

private:


};
#endif