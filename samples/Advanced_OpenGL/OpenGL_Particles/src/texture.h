#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

inline float RandomFloat()
{
	float Max = RAND_MAX;
	return ((float)rand() / Max);
}


/// <summary>
/// Creates a 1D random texture with 3 Channels.
/// Every Element is a vector of 3 floating point values.
/// </summary>
/// <param name="Size">The size of the texture.</param>
/// <returns>1D random texture</returns>
inline GLint Texture_RandomTexture(unsigned int Size)
{
	glm::vec3* pRandomData = new glm::vec3[Size];

	for (unsigned int i = 0; i < Size; i++) {
		pRandomData[i].x = RandomFloat();
		pRandomData[i].y = RandomFloat();
		pRandomData[i].z = RandomFloat();
	}

	GLuint id;

	glGenTextures(1, &id);
	glBindTexture(GL_TEXTURE_1D, id);
	glTexImage1D(GL_TEXTURE_1D, 0, GL_RGB, Size, 0.0f, GL_RGB, GL_FLOAT, pRandomData);
	glTexParameterf(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_REPEAT);	// set to repeat, to use any texture coordinate to access the texture

	delete[] pRandomData;

	return id;
};