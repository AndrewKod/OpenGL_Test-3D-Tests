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

////////////////////////////LIGHTS////////////////////////////
struct DirectionalLight {
    vec3 direction;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct PointLight {
    vec3 position;  
  
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
	
	//light fading
    float constant;
    float linear;
    float quadratic;

	bool bEnabled;
};

struct SpotLight {
    vec3  position;
    vec3  direction;
    float cutOff;
	float outerCutOff;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

#define NUM_POINT_LIGHTS 16
uniform DirectionalLight directionalLight;
uniform PointLight pointLights[NUM_POINT_LIGHTS];
uniform SpotLight spotLight;

float zNear = 0.1; 
float zFar  = 100.0; 

in vec3 FragPos;/*fragment shader's fragment coords*/
  
float LinearizeDepth(float depth);

vec3 CalcDirLight(vec3 normal, vec3 viewDir, vec3 diffuseColor, vec3 specularColor);

vec3 CalcPointLight(int lightId, vec3 normal, vec3 viewDir, vec3 diffuseColor, vec3 specularColor);

vec3 CalcSpotLight(vec3 normal, vec3 viewDir, vec3 diffuseColor, vec3 specularColor);

uniform bool bBlinn = true;//Blinn-Phong light model

///////////////////////NEW///////////////////////

out vec4 FragColor;

in vec2 TexCoords;
in vec3 Normal;
in vec3 Position;


uniform samplerCube skybox;
uniform vec3 cameraPos;

uniform bool bShowNormals = false;

uniform bool bReflect = false;

uniform bool binvertUVs = false;

layout (std140) uniform Settings
{
	bool bGammaCorrection;       
};

void main()
{   
	vec4 col = vec4(0.0);
	if(bShowNormals)
	{
		col = vec4(1.0, 1.0, 0.0, 1.0);
	}
	else if(bReflect)
	{
		vec3 resultColor = vec3(0.f);

		vec2 invertedTexCoords = vec2(TexCoords.x, binvertUVs ? 1.0 - TexCoords.y : TexCoords.y);

		vec3 diffuseColor = vec3(texture(material.diffuse[0], invertedTexCoords));
		float reflectionCoef = texture(material.specular[0], invertedTexCoords).r;

		vec3 I = normalize(Position - cameraPos);
		vec3 R = reflect(I, normalize(Normal));
		col = vec4(texture(skybox, R).rgb * reflectionCoef + diffuseColor, 1.0);
	}
	else
	{
		vec2 invertedTexCoords = vec2(TexCoords.x, binvertUVs ? 1.0 - TexCoords.y : TexCoords.y);

		vec3 diffuseColor = vec3(texture(material.diffuse[0], invertedTexCoords));
		
		col = vec4(diffuseColor, 1.0);
	}

	if(bGammaCorrection)
	{
		float gamma = 2.2;
		col = pow(col, vec4(1.0/gamma));
	}
	FragColor = col;
}

float LinearizeDepth(float depth) 
{
    // преобразуем обратно в NDC
    float z = depth * 2.0 - 1.0; 
    return (2.0 * zNear * zFar) / (zFar + zNear - z * (zFar - zNear));	
}

vec3 CalcDirLight(vec3 normal, vec3 viewDir, vec3 diffuseColor, vec3 specularColor)
{	
	vec3 directedLightDir = normalize(vec3(transView * vec4(-directionalLight.direction, 0.0)));
	
    // диффузное освещение
    float diff = max(dot(normal, directedLightDir), 0.0);

    // освещение зеркальных бликов
	float spec = 0.0;
    
	if(!bBlinn)
	{
		vec3 reflectDir = reflect(-directedLightDir, normal);
		spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
	}
	else
	{		
		vec3 halfwayDir = normalize(directedLightDir + viewDir);
		spec = pow(max(dot(normal, halfwayDir), 0.0), material.shininess * 2);
	}

    // комбинируем результаты
    vec3 ambient  = directionalLight.ambient  * diffuseColor;
    vec3 diffuse  = directionalLight.diffuse  * diff * diffuseColor;
    vec3 specular = directionalLight.specular * spec * specularColor;

    return (ambient + diffuse + specular);
}

vec3 CalcPointLight(int lightId, vec3 normal, vec3 viewDir, vec3 diffuseColor, vec3 specularColor)
{
	if(!pointLights[lightId].bEnabled)
		return vec3(0.f);

	/*Point Light Fading*/
	float dist = length(pointLights[lightId].position - FragPos);
	float attenuation = 1.0 / (pointLights[lightId].constant + pointLights[lightId].linear * dist + 
    		    pointLights[lightId].quadratic * (dist * dist));

	vec3 pointLightDir = normalize(pointLights[lightId].position - FragPos);	
    // диффузное освещение
    float diff = max(dot(normal, pointLightDir), 0.0);

    // освещение зеркальных бликов
	float spec = 0.0;
    
	if(!bBlinn)
	{
		vec3 reflectDir = reflect(-pointLightDir, normal);
		spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
	}
	else
	{		
		vec3 halfwayDir = normalize(pointLightDir + viewDir);
		spec = pow(max(dot(normal, halfwayDir), 0.0), material.shininess * 2);
	}
    
    // комбинируем результаты
    vec3 ambient  = pointLights[lightId].ambient  * diffuseColor;
    vec3 diffuse  = pointLights[lightId].diffuse  * diff * diffuseColor;
    vec3 specular = pointLights[lightId].specular * spec * specularColor;
    ambient  *= attenuation;
    diffuse  *= attenuation;
    specular *= attenuation;
    return (ambient + diffuse + specular);
} 

vec3 CalcSpotLight(vec3 normal, vec3 viewDir, vec3 diffuseColor, vec3 specularColor)
{	
	vec3 spotLightDir = normalize(-FragPos);/*projectedLightDir == viewDir*/
	
	/*projected light coeffs*/
	float theta = dot(spotLightDir, normalize(vec3(transView * vec4(-spotLight.direction, 0.0f))));
	float epsilon   = spotLight.cutOff - spotLight.outerCutOff;
	float intensity = clamp((theta - spotLight.outerCutOff) / epsilon, 0.0, 1.0);

	float spotDiff = 0.0f;
	if(theta > spotLight.outerCutOff) 
	{         
		spotDiff = max(dot(normal, spotLightDir), 0.0);
	}
	spotDiff *= intensity;		
	
	// освещение зеркальных бликов
	float spotSpec = 0.0;
    
	if(!bBlinn)
	{
		vec3 reflectSpotDir = reflect(-spotLightDir, normal);
		spotSpec = pow(max(dot(viewDir, reflectSpotDir), 0.0), material.shininess);
	}
	else
	{		
		vec3 halfwayDir = normalize(spotLightDir + viewDir);
		spotSpec = pow(max(dot(normal, halfwayDir), 0.0), material.shininess * 2);
	}


	spotSpec *= intensity;

	vec3 ambient  = spotLight.ambient * diffuseColor;
    vec3 diffuse  = spotLight.diffuse * spotDiff * diffuseColor;
    vec3 specular = spotLight.specular * spotSpec * specularColor;

    return (ambient + diffuse + specular);
}