#ifndef PARTICLES_H
#define PARTICLES_H

#include <GLFW/glfw3.h>
#include <vector>
#include "particle.h"

/// <summary>
/// 
/// </summary>
class ParticleSystem {
private:
	std::vector<Particle> particles;
	GLuint birthRate = 2;
	GLuint maxParticles = 10000;
	GLuint particleVAO;
	GLuint lastUsedParticle = 0;
	
	GLuint FirstUnusedParticle()
	{
		// Search from last used particle, this will usually return almost instantly
		for (GLuint i = lastUsedParticle; i < particles.size(); ++i) {
			if (particles[i].LifetimeMillis <= 0.0f) {
				lastUsedParticle = i;
				return i;
			}
		}

		// Otherwise, do a linear search
		for (GLuint i = 0; i < lastUsedParticle; ++i) {
			if (particles[i].LifetimeMillis <= 0.0f) {
				lastUsedParticle = i;
				return i;
			}
		}

		// create new Particle if 
		if (particles.size() < maxParticles) {
			particles.push_back(Particle());
			return particles.size() - 1;
		}


		// return -1 if maximum amount of particles is reached 
		return -1;
	}
	
	/// <summary>
	/// Respawns the particle.
	/// Resets Values
	/// </summary>
	void RespawnParticle(Particle & particle)
	{
		GLfloat random = ((rand()  % 100)-50) / 10.0f;
		GLfloat rColor = 0.5 + (rand() % 100) / 100;
		particle.P = glm::vec3(0) + random;
		particle.LifetimeMillis = 1;
		particle.v = glm::vec3(0,1,0) * 0.1f;	// initial vertical velocity
	}

public:
	ParticleSystem(GLuint nr_particles, glm::vec3 pos)
	{	// todo use pos
		for(GLuint i = 0; i < nr_particles; ++i)
			particles.push_back(Particle());

		// Set up mesh and attribute properties
		GLuint VBO;
		GLfloat particle_quad[] = {
			0.0f, 1.0f, 0.0f, 1.0f,
			1.0f, 0.0f, 1.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 0.0f,

			0.0f, 1.0f, 0.0f, 1.0f,
			1.0f, 1.0f, 1.0f, 1.0f,
			1.0f, 0.0f, 1.0f, 0.0f
		};
		glGenVertexArrays(1, &this->particleVAO);
		glGenBuffers(1, &VBO);
		glBindVertexArray(this->particleVAO);
		// Fill mesh buffer
		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(particle_quad), particle_quad, GL_STATIC_DRAW);
		// Set mesh attributes
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (GLvoid*)0);
		glBindVertexArray(0);
	}

	void update(float dt)
	{
		// add new particles
		for (GLuint i = 0; i < birthRate; i++)
		{
			int unusedParticle = FirstUnusedParticle();
			if(unusedParticle >= 0)
				RespawnParticle(particles[unusedParticle]);
		}

		// Update all particles 
		for (GLuint i = 0; i < particles.size(); ++i)
		{
			Particle& p = particles[i];
			p.LifetimeMillis -= dt; // reduce life
			if (p.LifetimeMillis > 0.0f)
			{

				// particle is alive, thus update
				p.P -= p.v * dt;
				// p.Cd.a -= dt * 2.5;	// decrease alpha, slowly disapear over time
			}
		}

	}

	void draw(Shader* shader) {
		glBlendFunc(GL_SRC_ALPHA, GL_ONE); // use GL_ONE blending mode to create glow effect
		shader->use();
		
		for (Particle particle : particles) {
			if (particle.LifetimeMillis > 0.0f)
			{
				shader->setVec3("offset", particle.P);
				//shader->setVec4("color", particle.Cd);
				glBindVertexArray(particleVAO);
				glDrawArrays(GL_TRIANGLES, 0, 6);
				glBindVertexArray(0);
			}
		}
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}
};

#endif // !PARTICLES_H