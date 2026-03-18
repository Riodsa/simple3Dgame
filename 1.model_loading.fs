#version 330 core
out vec4 FragColor;
in vec2 TexCoords;

uniform sampler2D texture_diffuse1;
uniform vec3 objectColor;
uniform bool useTexture; 

void main() {    
    if(useTexture) {
        FragColor = texture(texture_diffuse1, TexCoords) * vec4(objectColor, 1.0);
    } else {
        // If it's a shadow (black), give it transparency. 
        // If it's a wall, keep it solid.
        float alpha = (length(objectColor) < 0.1) ? 0.4 : 1.0; 
        FragColor = vec4(objectColor, alpha);
    }
}