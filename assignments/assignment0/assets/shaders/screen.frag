/* code is sourced from the Framebuffers tutorial on LearnOpenGL
	* https://learnopengl.com/Advanced-OpenGL/Framebuffers */

#version 450
out vec4 FragColor;
  
in vec2 TexCoords;

uniform sampler2D screenTexture;

void main() { 
    vec3 col = texture(screenTexture, TexCoords).rgb;
    FragColor = vec4(col, 1.0);
}