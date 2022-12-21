#ifndef WATER_H
#define WATER_H

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

#include "shader_m.h"

// ----------------------------------------------------------------------------
//
// Makros
//
// ----------------------------------------------------------------------------

#define WATER_RESOLUTION 100
#define WATER_SIZE glm::vec2(10)

// WatersimulationBuffers
// ----------------------
typedef struct {
	GLuint arrayBufferPositions;
	GLuint arrayBufferNormals;
} WaterSimulationBuffers;

// Water Simulation Program Locations
// ----------------------------------

// Uniform Locations
static GLuint waterSimulationProgram_VerticesPerRow = 0;
static GLuint waterSimulationProgram_Dt = 1;
static GLuint waterSimulationProgram_T = 2;
// Buffers Binding Locations 
static GLuint waterSimulationProgram_PositionBufferFront = 0;
static GLuint waterSimulationProgram_PositionBufferBack = 1;
static GLuint waterSimulationProgram_NormalBufferFront = 2;
static GLuint waterSimulationProgram_NormalBufferBack = 3;

// Water Drawing Program Locations
// -------------------------------


// Water Class 
// -----------
class Water {
public:
	// Water Class 
	Water() {
		waterFrontBuffer = 0;
		// Create Position & Normal Buffer
		water[0] = makeWaterSimulationBuffers(WATER_RESOLUTION, WATER_SIZE);
		water[1] = makeWaterSimulationBuffers(WATER_RESOLUTION, WATER_SIZE);

		// Water Compute Shader 
		waterSimulationProgram = new Shader(FileSystem::getPath("shader/watersimulation/watersimulation.comp").c_str());
		
		// Make Indices
		int numQuads = WATER_RESOLUTION * WATER_RESOLUTION;
		numElements = 6 * numQuads;

		std::vector<int> indices(numElements);

		for (int x = 0; x < WATER_RESOLUTION; ++x) {
			for (int z = 0; z < WATER_RESOLUTION; z++) {
				int idx = 6 * (z*WATER_RESOLUTION + x);

				indices[idx + 0] = z * numVerticesPerRow + x;
				indices[idx + 1] = z * numVerticesPerRow + x + 1;
				indices[idx + 2] = (z + 1) * numVerticesPerRow + x + 1;

				indices[idx + 3] = z * numVerticesPerRow + x;
				indices[idx + 4] = (z + 1) * numVerticesPerRow + x + 1;
				indices[idx + 5] = (z + 1) * numVerticesPerRow + x;
			}
		}
		
		// Make EBO 
		glGenBuffers(2, ebo);

		// Init EBO 
		for (int i = 0; i < 2; i++) {
			// Init EBO i
			glBindBuffer(GL_ARRAY_BUFFER, ebo[i]);
			glBufferData(GL_ARRAY_BUFFER, indices.size() * sizeof(int), indices.data(), GL_STREAM_DRAW);
		}

		glBindBuffer(GL_ARRAY_BUFFER, 0);

		// Make VAOs
		glGenVertexArrays(2, vao);

		for (int i = 0; i < 2; i++) {
			GLint previousVertexArrayObject;
			glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &previousVertexArrayObject);
			GLint previousArrayBuffer;
			glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &previousArrayBuffer);

			glBindVertexArray(vao[i]);
			glBindBuffer(GL_ARRAY_BUFFER, water[i].arrayBufferPositions);
			glEnableVertexAttribArray(0);
			glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, (void*)(intptr_t)0);
			glBindBuffer(GL_ARRAY_BUFFER, water[i].arrayBufferNormals);
			glEnableVertexAttribArray(1);
			glVertexAttribPointer(1, 3, GL_FLOAT, GL_TRUE, 0, (void*)(intptr_t)0);
			
			glBindBuffer(GL_ARRAY_BUFFER, previousArrayBuffer);
			glBindVertexArray(previousVertexArrayObject);
		}
	}

	void runWaterComputeShader(bool running, double t, double dt) {
		waterSimulationProgram->use();
		waterSimulationProgram->setInt(waterSimulationProgram_VerticesPerRow, numVerticesPerRow);
		waterSimulationProgram->setFloat(waterSimulationProgram_Dt, running ? dt : 0);
		waterSimulationProgram->setFloat(waterSimulationProgram_T, running ? t : 1);

		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, waterSimulationProgram_PositionBufferFront, water[waterFrontBuffer].arrayBufferPositions);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, waterSimulationProgram_PositionBufferBack, water[(waterFrontBuffer + 1)%2].arrayBufferPositions);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, waterSimulationProgram_NormalBufferFront, water[waterFrontBuffer].arrayBufferNormals);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, waterSimulationProgram_NormalBufferBack, water[(waterFrontBuffer + 1) % 2].arrayBufferNormals);

		glDispatchCompute(numVerticesPerRow, numVerticesPerRow, 1);

		glUseProgram(0);
	}

	void draw() {
		glBindVertexArray(vao[waterFrontBuffer]);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo[waterFrontBuffer]);
		glDrawElements(GL_TRIANGLES, numElements, GL_UNSIGNED_INT, NULL);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		glBindVertexArray(0);
	}

private:
	/* Member Variables */
	// Vertex Buffer Objects 
	WaterSimulationBuffers water[2];
	int waterFrontBuffer;
	//GLuint waterTexCoords;

	// Element Buffer Object 
	GLuint ebo[2];
	// Vertex Array Object
	GLuint vao[2];

	// Shader 
	// ------
	// Water Simulation Program (Compute Shader)
	Shader * waterSimulationProgram = NULL;

	// Count Vertices per Row
	int numVerticesPerRow;
	// Count Elements
	int numElements;

	/**
	 * Create Water Simulation Buffers
	 */
	WaterSimulationBuffers makeWaterSimulationBuffers(int numSubdivisions, glm::vec2 size) {
		WaterSimulationBuffers waterSimulationBuffers;

		numVerticesPerRow = numSubdivisions + 1;
		int numVertices = numVerticesPerRow * numVerticesPerRow;

		// Position 
		std::vector<glm::vec4> positions(numVertices);
		for (int x = 0; x < numVerticesPerRow; ++x) {
			for (int z = 0; z < numVerticesPerRow; ++z) {	
				int linearIndex = z * numVerticesPerRow + x;
				float relativeX = (float)x / (float)numSubdivisions;
				float relativeZ = (float)z / (float)numSubdivisions;
				
				// Minus, because Z-Axe poiting down
				glm::vec2 positionXZ = glm::vec2(-size.x / 2,
												  size.y / 2) +
									   glm::vec2(relativeX * size.x,
												-relativeZ * size.y);
				
				float height = 0;
				glm::vec4 position = glm::vec4(positionXZ.x, height, positionXZ.y, 0);

				positions[linearIndex] = position;
			}
		}

		// Normal 
		std::vector<glm::vec3> normals(numVertices);
		// TODO: KOORIGIEREN!!!!!
		for (int x = 0; x < numVerticesPerRow; ++x) {
			for (int z = 0; z < numVerticesPerRow; ++z) {
				int linearIndex = z * numVerticesPerRow + x;

				glm::vec3 position = positions[linearIndex];
				glm::vec3 positionEdge0;
				glm::vec3 positionEdge1;

				if (x+1 >= numVerticesPerRow) {
					positionEdge0 = positions[z*numVerticesPerRow + numVerticesPerRow - 2];
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

		// Create Buffers 
		// --------------
		// Position 
		glGenBuffers(1, &waterSimulationBuffers.arrayBufferPositions);
		glBindBuffer(GL_ARRAY_BUFFER, waterSimulationBuffers.arrayBufferPositions);
		glBufferData(GL_ARRAY_BUFFER, positions.size()* sizeof(glm::vec4), positions.data(), GL_STREAM_DRAW);
		
		// Normal
		glGenBuffers(1, &waterSimulationBuffers.arrayBufferNormals);
		glBindBuffer(GL_ARRAY_BUFFER, waterSimulationBuffers.arrayBufferNormals);
		glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(glm::vec3), normals.data(), GL_STREAM_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		return waterSimulationBuffers;
	}
};

#endif // !WATER_H
