/* code taken from the LearnOpenGL tutorial on Shadow Mapping
* https://learnopengl.com/Advanced-Lighting/Shadows/Shadow-Mapping */

#version 450

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoords;

out vec2 TexCoords;

void main()
{
    TexCoords = aTexCoords;
    gl_Position = vec4(aPos, 1.0);
}