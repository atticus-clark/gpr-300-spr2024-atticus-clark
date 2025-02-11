/* code taken from the LearnOpenGL tutorial on Shadow Mapping
* https://learnopengl.com/Advanced-Lighting/Shadows/Shadow-Mapping */

#version 450

out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D _DepthMap;
uniform float _NearPlane;
uniform float _FarPlane;

// required when using a perspective projection matrix
float LinearizeDepth(float depth)
{
    float z = depth * 2.0 - 1.0; // Back to NDC 
    return (2.0 * _NearPlane * _FarPlane) / (_FarPlane + _NearPlane - z * (_FarPlane - _NearPlane));	
}

void main()
{             
    float depthValue = texture(_DepthMap, TexCoords).r;
    // FragColor = vec4(vec3(LinearizeDepth(depthValue) / far_plane), 1.0); // perspective
    FragColor = vec4(vec3(depthValue), 1.0); // orthographic
}