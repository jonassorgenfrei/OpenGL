#ifndef LIGHT_H
#define LIGHT_H

#include <glad\glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <vector>

// Default Light values
const glm::vec3 POSITION(1.0f, 1.0f, 1.0f);
const glm::vec3 COLOR(1.0f, 1.0f, 1.0f);


// An abstract camera class that process input and calculates the corresponding 
// Eular Angles, Vectors and Matrices for use in OpenGl
class Light
{
public:
	glm::vec3 Position;
	glm::vec3 Color;

	//Constructor with vectors
	Light(glm::vec3 position = POSITION, glm::vec3 color = COLOR)
	{
		Position = position;
		Color = color;
	}	

	//Constructor with scalars
	Light(float x, float y, float z, float r, float g, float b)
	{
		Position = glm::vec3(x,y,z);
		Color = glm::vec3(r, g, b);
	}
private:
	

};
#endif