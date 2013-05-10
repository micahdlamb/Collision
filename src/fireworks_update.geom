#version 330

layout(points) in;
layout(points) out;
layout(max_vertices = 113) out;

in float Type0[];
in vec3 Position0[];
in vec3 Velocity0[];
in float Age0[];
in float Color0[];

out float Type1;
out vec3 Position1;
out vec3 Velocity1;
out float Age1;
out float Color1;

uniform float gDeltaTimeMillis;
uniform float gTime;
uniform sampler1D gRandomTexture;
uniform float gLauncherLifetime=500;
uniform float gShellLifetime=5600;
uniform float gSecondaryShellLifetime=6000;

#define PARTICLE_TYPE_LAUNCHER 0.0f
#define PARTICLE_TYPE_SHELL 1.0f
#define PARTICLE_TYPE_SECONDARY_SHELL 2.0f

vec3 GetRandomDir(float TexCoord)
{
    vec3 Dir = texture(gRandomTexture, TexCoord).xyz;
    Dir -= vec3(0.5, 0.5, 0.5);
    return Dir;
}

void main()
{
    float Age = Age0[0] + gDeltaTimeMillis;

    if (Type0[0] == PARTICLE_TYPE_LAUNCHER) {
        if (Age >= gLauncherLifetime) {
            Type1 = PARTICLE_TYPE_SHELL;
            Position1 = Position0[0];
            vec3 Dir = GetRandomDir(gTime/1000.0);
            Dir.y = max(Dir.y, 2);
            Velocity1 = normalize(Dir);
			Color1 = int(gTime) % 10;
		    Age1 = 0.0;
            EmitVertex();
            EndPrimitive();
			/*
			Age1 = 250.0;
			EmitVertex();
            EndPrimitive();
			Age1 = 500.0;
			EmitVertex();
            EndPrimitive();
			Age1 = 750.0;
			EmitVertex();
            EndPrimitive();
			Age1 = 1000.0;
			EmitVertex();
            EndPrimitive();
			*/
            Age = 0.0;
        }

        Type1 = PARTICLE_TYPE_LAUNCHER;
        Position1 = Position0[0];
        Velocity1 = Velocity0[0];
        Age1 = Age;
		Color1 = Color0[0];
        EmitVertex();
        EndPrimitive();
    } else {
        float DeltaTimeSecs = gDeltaTimeMillis / 1000.0f;
        float t1 = Age0[0] / 1000.0;
        float t2 = Age / 1000.0;
        vec3 DeltaP = DeltaTimeSecs * Velocity0[0];
        //vec3 DeltaV = vec3(DeltaTimeSecs) * (0.0, -9.81, 0.0);
		vec3 DeltaV = vec3(0.0, -.1, 0.0) * DeltaTimeSecs;
		Color1 = Color0[0];
		if (Type0[0] == PARTICLE_TYPE_SHELL) {
            if (Age < gShellLifetime) {
                Type1 = PARTICLE_TYPE_SHELL;
                Position1 = Position0[0] + DeltaP;
                Velocity1 = Velocity0[0] + DeltaV;
				Velocity1 -= Velocity1 * .3 * DeltaTimeSecs;
                Age1 = Age;
                EmitVertex();
                EndPrimitive();
				
				if (gShellLifetime - Age < 400){
					for (int i = 0 ; i < 112 ; i++) {
						Type1 = PARTICLE_TYPE_SECONDARY_SHELL;
						Position1 = Position0[0];
						vec3 Dir = GetRandomDir((gTime + i)/1000.0);
						Velocity1 = Velocity0[0] + normalize(Dir) / 3.0;
						//Velocity1 = Velocity0[0] + Dir / 2.0;
						Age1 = 0.0f;
						EmitVertex();
						EndPrimitive();
					}
				}
			}
        } else {
            if (Age < gSecondaryShellLifetime) {
                Type1 = PARTICLE_TYPE_SECONDARY_SHELL;
                Position1 = Position0[0] + DeltaP;
                Velocity1 = Velocity0[0] + DeltaV;
                Velocity1 -= Velocity1*.95*DeltaTimeSecs;
				Age1 = Age;
                EmitVertex();
                EndPrimitive();
            }
        }
    }
}