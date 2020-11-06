#version 330 core
struct DirectionalLight {
	//position for shadows calculations
	//vec4  position; //uncomment this field for behold a super-effect/////////
    vec4 direction;

    vec4 ambient;
    vec4 diffuse;
    vec4 specular;	
};

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

struct SpotLight {
    vec4  position;
    vec4  direction;
	
	vec4 ambient;
    vec4 diffuse;
    vec4 specular; 
	
	float cutOff;
	float outerCutOff;
  
};


#define NUM_POINT_LIGHTS 16


layout (std140) uniform Lights
{
	DirectionalLight directionalLight;
	SpotLight spotLight;
	PointLight pointLights[NUM_POINT_LIGHTS];
};

layout (std140) uniform PointLightRadiuses
{
	float pointLightRadiuses[NUM_POINT_LIGHTS];
};

layout (std140) uniform PointLightPositions
{
	vec4 pointLightPositions[NUM_POINT_LIGHTS];
};

uniform vec4 positions[NUM_POINT_LIGHTS];

out vec4 FragColor;
  
in vec2 TexCoords;

uniform sampler2D gPosition;
uniform sampler2D gModelNormal;
uniform sampler2D gMaterialNormal;
uniform sampler2D gAlbedoSpec;
uniform sampler2D gTangent;
uniform sampler2D gBitangent;


uniform vec3 cameraPos;
uniform float shininess = 32.0;

vec3 CalcDirLight(vec3 normal, vec3 viewDir, vec3 diffuseColor, vec3 specularColor, mat3 TBN);
vec3 CalcPointLight(int lightId, vec3 normal, vec3 viewDir, vec3 diffuseColor, vec3 specularColor,
	vec3 FragPos, mat3 TBN);

void main()
{ 	
	vec3 FragPos = texture(gPosition, TexCoords).rgb;
    vec3 ModelNormal = texture(gModelNormal, TexCoords).rgb;
	vec3 MaterialNormal = texture(gMaterialNormal, TexCoords).rgb;
	vec4 AlbedoSpec = texture(gAlbedoSpec, TexCoords);
    vec3 Albedo = AlbedoSpec.rgb;
    float Specular = AlbedoSpec.a;
	vec3 Tangent = texture(gTangent, TexCoords).rgb;
    vec3 Bitangent = texture(gBitangent, TexCoords).rgb;


	vec4 col = vec4(0.0);	

	mat3 TBN = transpose(mat3(Tangent, Bitangent, ModelNormal));   
	mat3 transTBN = mat3(Tangent, Bitangent, ModelNormal);
	

	vec3 viewDir = normalize(TBN * cameraPos - FragPos);//FragPos is already in tangent space

	//vec3 dirLigtColor = CalcDirLight(MaterialNormal, viewDir, Albedo, vec3(Specular), TBN);
	//col += vec4(dirLigtColor, 1.0);

	for(int lightId = 0; lightId < NUM_POINT_LIGHTS; lightId++)
	{			
		float dist = length(TBN * vec3(positions[lightId]) - FragPos);
        if(dist < pointLightRadiuses[lightId])
        {
			col += vec4(1.0);
			//vec3 lightDir = normalize(TBN * vec3(pointLights[lightId].position) - FragPos);
			//vec3 diffuse = max(dot(MaterialNormal, lightDir), 0.0) * Albedo * vec3(pointLights[lightId].diffuse);
			//col += vec4(diffuse, 0.0);

//			vec3 pointLightColor = 
//				CalcPointLight(lightId, MaterialNormal, viewDir, Albedo, vec3(Specular), FragPos, TBN);
//			col += vec4(pointLightColor, 1.0);
		}
	}	

	//FragColor = vec4(FragPos,1.0);
	FragColor = col;
}

vec3 CalcDirLight(vec3 normal, vec3 viewDir, vec3 diffuseColor, vec3 specularColor, mat3 TBN)
{	
	vec3 directedLightDir = normalize(TBN * vec3(-directionalLight.direction));
	
    // диффузное освещение
    float diff = max(dot(normal, directedLightDir), 0.0);

    // освещение зеркальных бликов
	float spec = 0.0;    
	
	vec3 halfwayDir = normalize(directedLightDir + viewDir);
	spec = pow(max(dot(normal, halfwayDir), 0.0), shininess * 2);
	

	
    float shadow = 0.0;
	// calculate shadow
	//if(bShadows)
		//shadow = DirLightShadowCalculation(normal, directedLightDir);

    // комбинируем результаты
    vec3 ambient  = vec3(directionalLight.ambient)  * diffuseColor;
    vec3 diffuse  = vec3(directionalLight.diffuse)  * diff * diffuseColor * (1.0 - shadow);
    vec3 specular = vec3(directionalLight.specular) * spec * specularColor * (1.0 - shadow);

	

    return (ambient + diffuse + specular);
}

vec3 CalcPointLight(int lightId, vec3 normal, vec3 viewDir, vec3 diffuseColor, vec3 specularColor,
	vec3 FragPos, mat3 TBN)
{
	if(!pointLights[lightId].bEnabled)
		return vec3(0.f);
	
	vec3 PointLightPosition =TBN * vec3(pointLights[lightId].position);

	/*Point Light Fading*/
	float dist = length(vec3(PointLightPosition) - FragPos);	

	float attenuation = 1.0 / (pointLights[lightId].constant + pointLights[lightId].linear * dist + 
    		    pointLights[lightId].quadratic * (dist * dist));

	vec3 pointLightDir = normalize(vec3(PointLightPosition) - FragPos);	
    // диффузное освещение
    float diff = max(dot(normal, pointLightDir), 0.0);

    // освещение зеркальных бликов
	float spec = 0.0;
    
	
	vec3 halfwayDir = normalize(pointLightDir + viewDir);
	spec = pow(max(dot(normal, halfwayDir), 0.0), shininess * 2);
	
    
	float shadow = 0.0;
	// calculate shadow
	//if(bShadows)
	//	shadow = PointLightShadowCalculation(normal, pointLightDir, lightId);		
    
	//clamp diffuse for HDR
	vec3 originDiff = vec3(0.0);
	
	originDiff = vec3(clamp(pointLights[lightId].diffuse, vec4(0.0), vec4(1.0)));	

	// комбинируем результаты
    vec3 ambient  = vec3(pointLights[lightId].ambient)  * diffuseColor;
    vec3 diffuse  = originDiff * diff * diffuseColor * (1.0 - shadow);
    vec3 specular = vec3(pointLights[lightId].specular) * spec * specularColor * (1.0 - shadow);
    ambient  *= attenuation;
    diffuse  *= attenuation;
    specular *= attenuation;
    return (ambient + diffuse + specular);
} 