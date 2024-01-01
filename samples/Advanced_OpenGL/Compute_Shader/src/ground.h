#ifndef GROUND_H
#define GROUND_H

// ----------------------------------------------------------------------------
//
// Include
//
// ----------------------------------------------------------------------------

#include <vector>

// OpenGL
#include <glad/glad.h>
// GLM 
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "modules/shader_m.h"

// Ground Class
// ------------
class Ground {

public:
	Ground() {
		int numSubdivisions = 100;
		glm::vec2 size(20.0f);
		glm::vec2 textureStretch(20.0);
			
		int numVerticesPerRow = numSubdivisions + 1;
		int numVertices = numVerticesPerRow * numVerticesPerRow;

		// Make Positions
		vector<glm::vec4> positions(numVertices);

		for (int x = 0; x < numVerticesPerRow; ++x) {
			for (int z = 0; z < numVerticesPerRow; ++z) {
				int linearIndex = z * numVerticesPerRow + x;

				float relativeX = (float)x / numSubdivisions;
				float relativeZ = (float)z / numSubdivisions;
				// Minus, because Z-Axe poiting down
				glm::vec2 positionXZ = glm::vec2(-size.x / 2, 
												  size.y / 2) + 
										glm::vec2(relativeX * size.x, 
											     -relativeZ * size.y);
				float height = heightFunction(positionXZ);
				glm::vec4 position = glm::vec4(positionXZ.x, height, positionXZ.y, 0);

				positions[linearIndex] = position;
			}
		}

		// Make Normals
		std::vector<glm::vec3> normals(numVertices);
		// TODO: KOORIGIEREN!!!!!
		for (int x = 0; x < numVerticesPerRow; ++x) {
			for (int z = 0; z < numVerticesPerRow; ++z) {
				int linearIndex = z * numVerticesPerRow + x;

				glm::vec3 position = glm::vec3(positions[linearIndex]);

				glm::vec3 positionEdge0;
				glm::vec3 positionEdge1;

				if (x + 1 >= numVerticesPerRow) {
					positionEdge0 = -positions[z*numVerticesPerRow + numVerticesPerRow - 2];
				}
				else {
					positionEdge0 = positions[z*numVerticesPerRow + x + 1];
				}

				if (z + 1 >= numVerticesPerRow) {
					positionEdge1 = positions[(numVerticesPerRow - 2)*numVerticesPerRow + x];
				}
				else {
					positionEdge1 = positions[(z + 1)*numVerticesPerRow + x];
				}

				glm::vec3 edge0 = positionEdge0 - position;
				glm::vec3 edge1 = positionEdge1 - position;

				glm::vec3 normal = glm::normalize(glm::cross(edge0, edge1));

				normals[linearIndex] = normal;
			}
		}

		// Make Tex Coords
		std::vector<glm::vec2> texCoords(numVertices);

		for (int x = 0; x < numVerticesPerRow; ++x) {
			for (int z = 0; z < numVerticesPerRow; ++z) {
				int linearIndex = z * numVerticesPerRow + x;

				float relativeX = (float)x / numSubdivisions;
				float relativeZ = (float)z / numSubdivisions;


				glm::vec2 texCoord = glm::vec2(
												relativeX * textureStretch.x,
												relativeZ * textureStretch.y
												);

				texCoords[linearIndex] = texCoord;
			}
		}

		// Make Indices
		numElements = 6 * numSubdivisions * numSubdivisions;

		std::vector<int> indices(numElements);

		for (int x = 0; x < numSubdivisions; ++x) {
			for (int z = 0; z < numSubdivisions; ++z) {
				int idx = 6 * (z*numSubdivisions + x);
				indices[idx + 0] = z * numVerticesPerRow + x;
				indices[idx + 1] = z * numVerticesPerRow + x + 1;
				indices[idx + 2] = (z + 1)*numVerticesPerRow + x + 1;

				indices[idx + 3] = z * numVerticesPerRow + x;
				indices[idx + 4] = (z + 1)*numVerticesPerRow + x + 1;
				indices[idx + 5] = (z + 1)*numVerticesPerRow + x;
			}
		}

		// generate Buffer
		// ---------------
		// position
		glGenBuffers(1, &positionsBuffer);
		glBindBuffer(GL_ARRAY_BUFFER, positionsBuffer);
		glBufferData(GL_ARRAY_BUFFER, positions.size()*sizeof(glm::vec4), positions.data(), GL_STREAM_DRAW);

		// normal
		glGenBuffers(1, &normalsBuffer);
		glBindBuffer(GL_ARRAY_BUFFER, normalsBuffer);
		glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(glm::vec3), normals.data(), GL_STREAM_DRAW);
		
		// texCoords
		glGenBuffers(1, &texCoordBuffer);
		glBindBuffer(GL_ARRAY_BUFFER, texCoordBuffer);
		glBufferData(GL_ARRAY_BUFFER, texCoords.size() * sizeof(glm::vec2), texCoords.data(), GL_STREAM_DRAW);

		// elementbuffer
		glGenBuffers(1, &ebo);
		glBindBuffer(GL_ARRAY_BUFFER, ebo);
		glBufferData(GL_ARRAY_BUFFER, indices.size() * sizeof(int), indices.data(), GL_STREAM_DRAW);

		glBindBuffer(GL_ARRAY_BUFFER, 0);

		// generate VAO
		// ------------
		glGenVertexArrays(1, &vao);

		// bind VBOs
		GLint previousVertexArrayObject;
		glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &previousVertexArrayObject);
		GLint previousArrayBuffer;
		glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &previousArrayBuffer);

		glBindVertexArray(vao);
		glBindBuffer(GL_ARRAY_BUFFER, positionsBuffer);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec4), (void*)(intptr_t)0);
		glBindBuffer(GL_ARRAY_BUFFER, normalsBuffer);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_TRUE, 0, (void*)(intptr_t)0);
		glBindBuffer(GL_ARRAY_BUFFER, texCoordBuffer);
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, (void*)(intptr_t)0);

		glBindBuffer(GL_ARRAY_BUFFER, previousArrayBuffer);
		glBindVertexArray(previousVertexArrayObject);
		
	}

	void draw() {
		glBindVertexArray(vao);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
		glDrawElements(GL_TRIANGLES, numElements, GL_UNSIGNED_INT, NULL);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		glBindVertexArray(0);
	}

private:
	// Vertex Buffer Objects
	GLuint positionsBuffer;
	GLuint normalsBuffer;
	GLuint texCoordBuffer;
	// Element Buffer Object
	GLuint ebo;
	// Vertex Array Object 
	GLuint vao;
	// count elements 
	int numElements;

	glm::vec2 floorv(glm::vec2 v) {
		return glm::vec2(
			(float)floor((float)v.x),
			(float)floor((float)v.y)
			);
	}

	glm::vec2 fracv(glm::vec2 v) {
		glm::vec2 f = floorv(v);
		return v - f;
	}

	float rand2(glm::vec2 xi) {
		float cos = glm::dot(xi, glm::vec2(12.9898, 78.233));
		float value = sin(cos) * 43758.5453;
		return value - floor(value);
	}

	float lerpf(float x, float y, float alpha) {
		return (1.0f - alpha) * x + alpha * y;
	}

	float octave(glm::vec2 v, float wavelength, float amplitude) {
		v = v / glm::vec2(wavelength);

		glm::vec2 frac = fracv(v);
		glm::vec2 floor = floorv(v);

		float leftLower = rand2(glm::vec2(floor.x, floor.y));
		float rightLower = rand2(glm::vec2(floor.x + 1, floor.y));
		float rightUpper = rand2(glm::vec2(floor.x + 1, floor.y + 1));
		float leftUpper = rand2(glm::vec2(floor.x, floor.y + 1));

		float lower = lerpf(leftLower, rightLower, frac.x);
		float upper = lerpf(leftUpper, rightUpper, frac.x);

		return amplitude * lerpf(lower, upper, frac.y);
	}

	float heightFunction(glm::vec2 position) {
		position = position*glm::vec2(3);

		float value = 0;
		float wavelength = 16;
		int index = 0;
		float persistance = 0.7f;

		while (wavelength > 1) {
			float amplitude = pow(persistance, index);
			value += octave(position, wavelength, amplitude);
			wavelength /= 2;
			index++;
		}

		return (-0.9f + min(0.8f, glm::length(position*glm::vec2(0.05)))) + value;
	}
};


#endif // !GROUND
