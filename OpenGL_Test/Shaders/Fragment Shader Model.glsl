#version 330 core

#define NUM_TEXTURE_MAPS 16

struct Material {
    /*vec3 ambient;*//*ambient in most of cases matches with diffuse*/
    sampler2D diffuse[NUM_TEXTURE_MAPS];
    sampler2D specular[NUM_TEXTURE_MAPS];
	sampler2D emissive[NUM_TEXTURE_MAPS];
	sampler2D reflection[NUM_TEXTURE_MAPS];
    float shininess;
}; 
  
uniform Material material;


///////////////////////NEW///////////////////////

out vec4 FragColor;

in vec2 TexCoords;
in vec3 Normal;
in vec3 Position;


uniform samplerCube skybox;
uniform vec3 cameraPos;

void main()
{   
	vec3 resultColor = vec3(0.f);

	vec2 invertedTexCoords = vec2(TexCoords.x, 1.0 -TexCoords.y);

	vec3 diffuseColor = vec3(texture(material.diffuse[0], invertedTexCoords));
	float reflectionCoef = texture(material.specular[0], invertedTexCoords).r;

	vec3 I = normalize(Position - cameraPos);
	vec3 R = reflect(I, normalize(Normal));
	FragColor = vec4(texture(skybox, R).rgb * reflectionCoef + diffuseColor, 1.0);
}