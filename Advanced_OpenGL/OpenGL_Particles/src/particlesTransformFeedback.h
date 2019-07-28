#ifndef PARTICLES_TF_H
#define PARTICLES_TF_H

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "shader_m.h"

/* Use Transform Feedback Buffer */

/// A particle can either be a launcher, a shell or a secondary shell
/// Launcher - creates periodically shell particles and fires them upwards (doesnt have a lifetime)
/// Shell - explodes after a few seconds into secondary shells that fly in random directions
/// Gravity is a force that influences the particle
enum PType { PARTICLE_TYPE_NONE, PARTICLE_TYPE_LAUNCHER };

/// <summary>
/// Structure for each particle
/// </summary>
struct ParticleV2
{
	PType Type;
	glm::vec3 P; // Position
	glm::vec3 v; // Velocity
	float LifetimeMillis;	// life time in ms

	ParticleV2(): Type(PARTICLE_TYPE_NONE), P(0),v(0),LifetimeMillis(0)
	{}
};

class PartcilesSystemTf {
public:
	/// <summary>
	/// Initializes a new instance of the <see cref="PartcilesSystemTf"/> class.
	/// </summary>
	/// <param name="nr_particles">The nr particles.</param>
	/// <param name="pos">The position.</param>
	PartcilesSystemTf(GLuint nr_particles, glm::vec3 pos)
	{
		this->particleCount = nr_particles;
		for (GLuint i = 0; i < this->particleCount; ++i)
			particles.push_back(ParticleV2());

		particles[0].Type = PARTICLE_TYPE_NONE;
		particles[0].P = pos;
		particles[0].v = glm::vec3(0, 0.0001f, 0);
		particles[0].LifetimeMillis = 0.0f;

		glGenTransformFeedbacks(2, m_transformFeedback);
		glGenBuffers(2, m_particleBuffer);

		for (unsigned int i = 0; i < 2; i++)
		{
			glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, m_transformFeedback[i]);
			glBindBuffer(GL_ARRAY_BUFFER, m_particleBuffer[i]);
			glBufferData(GL_ARRAY_BUFFER, sizeof(ParticleV2)*particles.size(), particles.data(), GL_DYNAMIC_DRAW);
			glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, m_particleBuffer[i]);
		}

	}

	/**
	 * @param dt - delta time from the previous call in ms
	 */
	void update(float dt)
	{
	}

	void draw(Shader* shader)
	{
	}
private:
	unsigned int particleCount;
	unsigned int maxParticles = 10000;

	std::vector<ParticleV2> particles;

	bool m_isFirst;

	unsigned int m_currVB; // specifies which Vertex Buffer is current
	unsigned int m_currTFB; // specifies which Transform Feedback object is current

	GLuint m_particleBuffer[2];	// handles for the vertex buffers
	GLuint m_transformFeedback[2];	// handles for the transform feedback object

	//PSUpdateTechnique m_updateTechnique;
	//BillboardTechnique m_billboardTechnique;

	//RandomTexture m_randomTexture;
	//Texture* m_pTexture;	// texture that contains random numbers
	int m_time;	// current global time variable
};

#endif // !PARTICLES_TF_H