#version 430 core

// ----------------------------------------------------------------------------
//
// Attributes
//
// ----------------------------------------------------------------------------

layout (location = 0) in vec3 vPosition;    
layout (location = 1) in vec3 vNormal;      
layout (location = 2) in vec2 vTexCoord;    

out vec4 fWorldPosition;                    
out vec3 fWorldNormal;                      
out vec2 fTexCoord;                         

// ----------------------------------------------------------------------------
//
// Uniforms
//
// ----------------------------------------------------------------------------

uniform mat4 ModelMatrix;                   
uniform mat3 NormalMatrix;                  
uniform mat4 ViewMatrix;                    
uniform mat4 ProjectionMatrix;              

// ----------------------------------------------------------------------------
//
// Functions
//
// ----------------------------------------------------------------------------

void main() {
    fWorldPosition = ModelMatrix*vec4(vPosition, 1.0);
    fWorldNormal = normalize(NormalMatrix*vNormal);
    fTexCoord = vTexCoord;
    gl_Position = ProjectionMatrix*ViewMatrix*fWorldPosition;
}
