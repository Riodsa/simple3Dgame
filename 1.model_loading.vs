#version 330 core
layout (location = 0) in vec3 aPos;       // The position of the vertex
layout (location = 1) in vec3 aNormal;    // The normal vector (for lighting)
layout (location = 2) in vec2 aTexCoords; // The UV coordinates for textures

out vec2 TexCoords; // Pass this to the Fragment Shader

uniform mat4 model;      // Translates/Scales the object in the world
uniform mat4 view;       // The camera's position and orientation
uniform mat4 projection; // The perspective/zoom of the camera

void main()
{
    // Pass the texture coordinates directly to the fragment shader
    TexCoords = aTexCoords;    
    
    // Calculate the final position of the vertex on the screen
    // Math: Projection * View * Model * localPosition
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}