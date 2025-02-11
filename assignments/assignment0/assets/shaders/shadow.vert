/* code taken from the LearnOpenGL tutorial on Shadow Mapping
* https://learnopengl.com/Advanced-Lighting/Shadows/Shadow-Mapping */

#version 450

layout (location = 0) in vec3 aPos;

uniform mat4 _LightSpaceMatrix;
uniform mat4 _Model;

void main()
{
    gl_Position = _LightSpaceMatrix * _Model * vec4(aPos, 1.0);
}
