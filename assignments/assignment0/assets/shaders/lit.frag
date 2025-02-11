#version 450

out vec4 FragColor; // The color of this fragment

in Surface {
	vec3 WorldPos; // Vertex position in world space
	vec3 WorldNormal; // Vertex normal in world space
	vec2 TexCoord;
	vec4 FragPosLightSpace;
} fs_in;

uniform sampler2D _MainTex; 
uniform vec3 _EyePos;
uniform vec3 _LightColor = vec3(1.0);
uniform vec3 _AmbientColor = vec3(0.3,0.4,0.46);

struct Material {
	float Ka; // Ambient coefficient (0-1)
	float Kd; // Diffuse coefficient (0-1)
	float Ks; // Specular coefficient (0-1)
	float Shininess; // Affects size of specular highlight
};
uniform Material _Material;

/* code taken from the LearnOpenGL tutorial on Shadow Mapping
* https://learnopengl.com/Advanced-Lighting/Shadows/Shadow-Mapping */

uniform sampler2D _DiffuseTexture;
uniform sampler2D _ShadowMap;

uniform vec3 _LightPos;

float ShadowCalculation(vec4 fragPosLightSpace)
{
	// perform perspective divide
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    // transform to [0,1] range
    projCoords = projCoords * 0.5 + 0.5;
    // get closest depth value from light's perspective (using [0,1] range fragPosLight as coords)
    float closestDepth = texture(_ShadowMap, projCoords.xy).r; 
    // get depth of current fragment from light's perspective
    float currentDepth = projCoords.z;
    // check whether current frag pos is in shadow
    float shadow = currentDepth > closestDepth ? 1.0 : 0.0;

    return shadow;
}

void main() {
	vec3 color = texture(_DiffuseTexture, fs_in.TexCoord).rgb;
    vec3 normal = normalize(fs_in.WorldNormal);

    // ambient
    vec3 ambient = _Material.Ka * _AmbientColor;

    // diffuse
    vec3 lightDir = normalize(_LightPos - fs_in.WorldPos);
    float diff = max(dot(lightDir, normal), 0.0);
    vec3 diffuse = _Material.Kd * diff * _LightColor;

    // specular
    vec3 viewDir = normalize(_EyePos - fs_in.WorldPos);
    float spec = 0.0;
    vec3 halfwayDir = normalize(lightDir + viewDir);
    spec = pow(max(dot(normal, halfwayDir), 0.0), _Material.Shininess);
    vec3 specular = _Material.Ks * spec * _LightColor;

    // calculate shadow
    //float shadow = 0.0f; // enable this and disable next line to turn off shadows
    float shadow = ShadowCalculation(fs_in.FragPosLightSpace);
    vec3 lighting = (ambient + (1.0 - shadow) * (diffuse + specular)) * color;
    
    FragColor = vec4(lighting, 1.0);
}
