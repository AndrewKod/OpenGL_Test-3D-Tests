#version 330 core
out vec4 FragColor;
  
in vec2 TexCoords;


uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gAlbedoSpec;
uniform sampler2D gTangent;
uniform sampler2D gBitangent;

struct PointLight {
    vec4 position;  
  
    vec4 ambient;
    vec4 diffuse;
    vec4 specular;
	
	//light fading
    float constant;
    float linear;
    float quadratic;

	bool bEnabled;
};
const int NR_LIGHTS = 32;
uniform PointLight pointLights[NR_LIGHTS];
uniform vec3 cameraPos;

void main()
{ 	
	vec3 FragPos = texture(gPosition, TexCoords).rgb;
    vec3 Normal = texture(gNormal, TexCoords).rgb;
    vec3 Albedo = texture(gAlbedoSpec, TexCoords).rgb;
    float Specular = texture(gAlbedoSpec, TexCoords).a;
	vec3 Tangent = texture(gTangent, TexCoords).rgb;
    vec3 Bitangent = texture(gBitangent, TexCoords).rgb;


	vec4 col = vec4(0.0);


	FragColor = col;
}