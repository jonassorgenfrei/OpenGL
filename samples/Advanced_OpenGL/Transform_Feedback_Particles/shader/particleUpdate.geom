#version 330 core

layout(points) in;	// get points as input
layout(points) out; // provide points as output
layout(max_vertices = 256) out;

in int Type0[];
in vec3 Position0[];
in vec3 Velocity0[];
in float Age0[];

/* Outputs to the transform feedback buffer */
out int Type1;
out vec3 Position1;
out vec3 Velocity1;
out float Age1;

uniform float gDeltaTimeMillis;
uniform float gTime;
uniform sampler1D gRandomTexture;
uniform float gLauncherLifetime;	// life time of the launcher
uniform float gShellLifetime;		// lifetime of the shell
uniform float gSecondaryShellLifetime;	// lifetime of the secondary shell

#define PARTICLE_TYPE_LAUNCHER 0
#define PARTICLE_TYPE_SHELL 1
#define PARTICLE_TYPE_SECONDARY_SHELL 2

/**
 * Use to generate a random direction for the shells.
 */
vec3 GetRandomDir(float TexCoord)
{
    vec3 Dir = texture(gRandomTexture, TexCoord).xyz; // samples the texture with a floating point value
    Dir -= vec3(0.5, 0.5, 0.5);	// subtract the 0.5 since  all values are in range [0,1]
    return Dir;	// range [-0.5, -.5], allows the particles to fly in all directions
}

void main()
{
	float Age = Age0[0] + gDeltaTimeMillis;	// update particle age

	if (Type0[0] == PARTICLE_TYPE_LAUNCHER) {	// for Launcher particles
		if (Age >= gLauncherLifetime) {	// check if Launcher particle lifetime has expired
            // emit shell particle
			// -------------------
			Type1 = PARTICLE_TYPE_SHELL;	
            // get position of the emitter particle

			Position1 = Position0[0];	
            
			// calc random direction
			vec3 Dir = GetRandomDir(gTime/1000.0); // use global time as pseudo random seed
            Dir.y = max(Dir.y, 0.5);	// set minimum y-Dir to 0.5, so it emitts into the sky
            Velocity1 = normalize(Dir) / 20.0;	// normalize direction vector and divide by 20

            Age1 = 0.0;

            EmitVertex();	
            EndPrimitive();

            Age = 0.0;	// reset launcher age to 0
		}

		// always output the launcher itself back into the buffer
		// (else no more particles will be created)
        Type1 = PARTICLE_TYPE_LAUNCHER;
        Position1 = Position0[0];
        Velocity1 = Velocity0[0];
        Age1 = Age;
        EmitVertex();
        EndPrimitive();
    } else {
		// translate milliseconds to seconds
        float DeltaTimeSecs = gDeltaTimeMillis / 1000.0f;
		// translate the old age of the particle and the new age to seconds as well
        float t1 = Age0[0] / 1000.0;	// old age in seconds
        float t2 = Age / 1000.0;		// new age in seconds
        
		// use Eulerintegration to calculate Position offset
		vec3 DeltaP = DeltaTimeSecs * Velocity0[0];
		// calculate new Velocity (Eulerintegration)
        vec3 DeltaV = vec3(DeltaTimeSecs) * (0.0, -9.81, 0.0); 

		if (Type0[0] == PARTICLE_TYPE_SHELL) {	// the shell particle
			if (Age < gShellLifetime) {	// as long as it's living
				Type1 = PARTICLE_TYPE_SHELL;
				Position1 = Position0[0] + DeltaP;	// update position
				Velocity1 = Velocity0[0] + DeltaV;	// update velocity
				Age1 = Age;
				EmitVertex();	// emit the particle again
				EndPrimitive();
			} else {	// if particle is dead
				for (int i = 0 ; i < 100; i++) {	// emit 10 secondary particles
					Type1 = PARTICLE_TYPE_SECONDARY_SHELL;

					Position1 = Position0[0];	// get position of the parent

					// calculate random velocity for each particle (NOTE: DONT LIMIT THE VELOCITY)
					vec3 Dir = GetRandomDir((gTime + i)/1000.0);
					Velocity1 = normalize(Dir) / 20.0;

					Age1 = 0.0f;
					EmitVertex();
					EndPrimitive();
				}
			}
		} else {	// the secondary shell particle 
            if (Age < gSecondaryShellLifetime) {	// update the particle as long as it lives
                Type1 = PARTICLE_TYPE_SECONDARY_SHELL;
                Position1 = Position0[0] + DeltaP;
                Velocity1 = Velocity0[0] + DeltaV;
                Age1 = Age;
                EmitVertex();
                EndPrimitive();
            }	// when dead don't emit new particle
		}
    }
} 