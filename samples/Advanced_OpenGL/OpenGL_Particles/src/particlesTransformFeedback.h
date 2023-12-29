#ifndef PARTICLES_TF_H
#define PARTICLES_TF_H

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "filesystem.h"
#include "particle.h"
#include "shader_m.h"
#include "texture.h"

#define MAX_PARTICLES 1000
#define PARTICLE_LIFETIME 10.0f

/* Use Transform Feedback Buffer */

class ParticleSystemTF {
public:
	/// <summary>
	/// Initializes a new instance of the <see cref="PartcilesSystemTf"/> class.
	/// </summary>
	/// <param name="nr_particles">The nr particles.</param>
	/// <param name="pos">The position.</param>
	ParticleSystemTF(GLuint nr_particles, glm::vec3 pos)
	{	
		// setup storage for particles
		for (GLuint i = 0; i < ((nr_particles < MAX_PARTICLES) ? nr_particles : MAX_PARTICLES); ++i)
			particles.push_back(Particle());

		// initialize first particle as (static) launcher 
		particles[0].Type = PARTICLE_TYPE_LAUNCHER;	
		particles[0].P = pos; // use position of particle system as starting point
		particles[0].v = glm::vec3(0, 0.0001f, 0);
		particles[0].LifetimeMillis = 0.0f;

		glGenTransformFeedbacks(2, m_transformFeedback);
		glGenBuffers(2, m_particleBuffer);

		for (unsigned int i = 0; i < 2; i++)
		{
			// bind transform feedback object to the GL_TRANSFORM_FEEDBACK target
			// following operations are perfomed on this
			glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, m_transformFeedback[i]);
			// bind corresponding buffer obj
			glBindBuffer(GL_ARRAY_BUFFER, m_particleBuffer[i]);
			// load particle array into buffer
			glBufferData(GL_ARRAY_BUFFER, sizeof(Particle)*particles.size(), particles.data(), GL_DYNAMIC_DRAW);
			// bind corr. buffer obj to GL_TRANSFORM_FEEDBACK_BUFFER and place it as index 0
			// e.g. having primitives redirect into more than one buffer by binding 
			// serveral buffers at different indices
			glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, m_particleBuffer[i]);
		}

		// init random texture
		// -------------------
		randomTexture = Texture_RandomTexture(1000);

		// Setup update shader
		// -------------------
		
		particleUpdateShader = new Shader(FileSystem::getPath("shader/particleUpdate.vert").c_str(), FileSystem::getPath("shader/particleUpdate.frag").c_str(), FileSystem::getPath("shader/particleUpdate.geom").c_str());
		//particleUpdateShader->link();

		const GLchar* Varyings[4]; // array of strings with the name of the attributes
		// theses are the output attributes from the last shader before the FS
		Varyings[0] = "Type1";
		Varyings[1] = "Position1";
		Varyings[2] = "Velocity1";
		Varyings[3] = "Age1";

		// specify attributes that go into the buffer, before the programm is linked!
		/* TODO CHECK !!! */
		/**
		 * The last parameter to glTransformFeedbackVaryings() tells
		 * OpenGL either to write all the attributes as a single structure
		 * into a single buffer (GL_INTERLEAVED_ATTRIBS). Or to dedicate
		 * a single buffer for each attribute (GL_SEPARATE_ATTRIBS).
		 * If you use GL_INTERLEAVED_ATTRIBS you can only have a single
		 * transform feedback buffer bound. If you use GL_SEPARATE_ATTRIBS
		 * you will need to bind a different buffer to each slot
		 * (according to the number of attributes)
		 */
		glTransformFeedbackVaryings(particleUpdateShader->ID, 4, Varyings, GL_INTERLEAVED_ATTRIBS);
		
		particleUpdateShader->link();
		

		// Set shader state
		// ----------------
		particleUpdateShader->use(); 
		particleUpdateShader->setInt("gRandomTexture", 0);
		particleUpdateShader->setFloat("gLauncherLifetime", 1000.0f);// life time of the launcher
		particleUpdateShader->setFloat("gShellLifetime", 10000.0f);// lifetime of the shell
		particleUpdateShader->setFloat("gSecondaryShellLifetime", 10000.0f);// lifetime of the secondary shell

	}

	/**
	 * @param dt - delta time from the previous call in ms
	 */
	void update(float dt)
	{

		m_time += dt*1000;	// updating global time counter

		// Update Particles
		particleUpdateShader->use();	// enable update shader and setting dynamic state into it
		
		particleUpdateShader->setFloat("gTime", m_time);	// global time as semi random seed
		particleUpdateShader->setFloat("gDeltaTimeMillis", dt * 1000); // set amount of time that has passed from previous render
		
		// use random texture to provide directions for the generated particles 
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_1D, randomTexture);
		
		// prevent renderer to rasterize screen & cut the flow 
		// it tells the pipeline to discard all primtives before they reach the rasterizer
		glEnable(GL_RASTERIZER_DISCARD);

		// handles the toggeling 
		glBindBuffer(GL_ARRAY_BUFFER, m_particleBuffer[currVB]);	// bind input 
		glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, m_transformFeedback[currTFB]); // bind output, (brings along the attached state, the actual buffer)

		// setup the vertex attributes of the particles in the vertex buffer
		glEnableVertexAttribArray(0); 
		glEnableVertexAttribArray(1);
		glEnableVertexAttribArray(2);
		glEnableVertexAttribArray(3);
		
		glVertexAttribPointer(0, 1, GL_FLOAT, GL_FALSE, sizeof(Particle), 0); // type /* TODO!!! CHECK */
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Particle), (const GLvoid*)4); // position
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Particle), (const GLvoid*)16); // velocity
		glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(Particle), (const GLvoid*)28); // lifetime

		// make transform feedback active
		// all draw calls after that and until glEndTransformFeedback() is called redirect their output to the 
		// transform feedback buffer according to the currently bound transform feedback object
		glBeginTransformFeedback(GL_POINTS);	// topology parameter: points (only completet primtives can be written into buffer)
		
		if (m_isFirst) { // for the first call we know that only one primtive has to be drawn
			// transform feedback buffer, doesnt habe any record of previous transform feedback activity in addition
			glDrawArrays(GL_POINTS, 0, 1);	// first call has to be explicitly
			m_isFirst = false;
		}
		else {
			/**
			 * first parameter -  topology
			 * second parameter - transform feedback obj to which the current vertex buffer is attached
			 */
			glDrawTransformFeedback(GL_POINTS, m_transformFeedback[currVB]);
			// note we can't tell the transformFeedback how many particles to process
			// checks the input buffer and draws vertices that have been previously written into the it
		}
			
		glEndTransformFeedback();

		glDisableVertexAttribArray(0);
		glDisableVertexAttribArray(1);
		glDisableVertexAttribArray(2);
		glDisableVertexAttribArray(3);

		// re-Enable Rasterization
		glDisable(GL_RASTERIZER_DISCARD);
	}

	void draw(Shader* shader)
	{
		// Render Particles
		shader->use();	// enable draw shader

		shader->setFloat("gBillboardSize", 0.01f);

		// bind current Buffer for reading which provides the input vertices for rendering
		glBindBuffer(GL_ARRAY_BUFFER, m_particleBuffer[currTFB]);

		// only need position from buffer 
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Particle), (const GLvoid*)4); // position
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, sizeof(Particle), (const GLvoid*)0); // position

		// use this function again in order to draw
		glDrawTransformFeedback(GL_POINTS, m_transformFeedback[currTFB]); // the object knows how many vertices to draw
		
		glDisableVertexAttribArray(0);

		// Toggeling between the two buffers
		// (m_currVB == current vertex Buffer init 0 | m_currTFB == current transform feedback buffer init 1)
		currVB = currTFB;
		currTFB = (currTFB + 1) & 0x1;	// mcurrTFB is allway opposing to m_currVB
	}


private:
	std::vector<Particle> particles;

	bool m_isFirst = true;

	unsigned int currVB = 0; // specifies which Vertex Buffer is current
	unsigned int currTFB = 1; // specifies which Transform Feedback object is current

	GLuint m_particleBuffer[2];	// handles for the vertex buffers
	GLuint m_transformFeedback[2];	// handles for the transform feedback object

	Shader* particleUpdateShader;	// handles the update of the particles

	GLuint randomTexture; // texture that contains random numbers
	int m_time = 0;	// current global time variable

};

#endif // !PARTICLES_H