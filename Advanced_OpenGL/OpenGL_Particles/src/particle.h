#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

/// A particle can either be a launcher, a shell or a secondary shell
/// Launcher - creates periodically shell particles and fires them upwards (doesnt have a lifetime)
/// Shell - explodes after a few seconds into secondary shells that fly in random directions
/// Gravity is a force that influences the particle
//enum PType { PARTICLE_TYPE_LAUNCHER, PARTICLE_TYPE_SHELL, PARTICLE_TYPE_SECONDARY_SHELL };
#define PARTICLE_TYPE_LAUNCHER 0
/// <summary>
/// Structure for each particle
/// </summary>
struct Particle
{
	int Type;			// TYPE FLOAT????
	glm::vec3 P;		// Position
	glm::vec3 v;		// Velocity
	float LifetimeMillis;	// life time in ms

	//Particle() : Type(PARTICLE_TYPE_LAUNCHER), P(0), v(0), LifetimeMillis(0)
	//{}
};
