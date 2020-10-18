#version 330 core

#define NUM_TEXTURE_MAPS 16

struct Material {
    /*vec3 ambient;*//*ambient in most of cases matches with diffuse*/
    sampler2D diffuse[NUM_TEXTURE_MAPS];
    sampler2D specular[NUM_TEXTURE_MAPS];
	sampler2D emissive[NUM_TEXTURE_MAPS];
	sampler2D reflection[NUM_TEXTURE_MAPS];
	sampler2D normal[NUM_TEXTURE_MAPS];
    float shininess;
}; 
  
uniform Material material;

////////////////////////////LIGHTS///////////////////

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


uniform bool bStencil = false;
uniform vec4 borderColor;

uniform samplerCube skybox;
uniform vec3 cameraPos;



uniform bool bReflect = false;
uniform bool bRefract = false;

uniform bool binvertUVs = false;

uniform bool bShowNormals;

layout (std140) uniform Settings
{
	bool bGammaCorrection;       
	
	bool bExplode;
	bool bPostProcess;

	bool bAntiAliasing;
	bool bBlit;

	bool bPointLights;
	bool bDirectionalLight;
	bool bSpotLight;

	bool bShadows;
};

#define NUM_POINT_LIGHTS 16

layout (std140) uniform Lights
{
	DirectionalLight directionalLight;
	SpotLight spotLight;
	PointLight pointLights[NUM_POINT_LIGHTS];
};


//////////////////Shadows//////////////////

//Dir light shadows
uniform sampler2D dirLight_ShadowMap;
in vec4 FragPosDirLightSpace;

float DirLightShadowCalculation(vec3 normal, vec3 lightDir);

//point lights shadows
uniform samplerCube pointLight_ShadowMaps[NUM_POINT_LIGHTS];

uniform float far_plane;

float PointLightShadowCalculation(vec3 normal, vec3 lightDir, int lightId);

vec3 sampleOffsetDirections[20] = vec3[]
(
   vec3( 1,  1,  1), vec3( 1, -1,  1), vec3(-1, -1,  1), vec3(-1,  1,  1), 
   vec3( 1,  1, -1), vec3( 1, -1, -1), vec3(-1, -1, -1), vec3(-1,  1, -1),
   vec3( 1,  1,  0), vec3( 1, -1,  0), vec3(-1, -1,  0), vec3(-1,  1,  0),
   vec3( 1,  0,  1), vec3(-1,  0,  1), vec3( 1,  0, -1), vec3(-1,  0, -1),
   vec3( 0,  1,  1), vec3( 0, -1,  1), vec3( 0, -1, -1), vec3( 0,  1, -1)
);   



void main()
{   
	vec2 texCoords = vec2(TexCoords.x, binvertUVs ? 1.0 - TexCoords.y : TexCoords.y);
	vec3 diffuseColor = vec3(texture(material.diffuse[0], texCoords));
	

	vec4 col = vec4(0.0);
	if(bStencil)
	{
		col = borderColor;
	}	
	else if(bShowNormals)
	{
		col = vec4(1.0, 1.0, 0.0, 1.0);
	}
	else if(bReflect)
	{		
		float reflectionCoef = texture(material.specular[0], texCoords).r;
		vec3 I = normalize(Position - cameraPos);
		vec3 R = reflect(I, normalize(Normal));
		col = vec4(texture(skybox, R).rgb * reflectionCoef + diffuseColor, 1.0);
	}
	else if(bRefract)
	{
		float ratio = 1.00 / 1.52;
		vec3 I = normalize(Position - cameraPos);
		vec3 R = refract(I, normalize(Normal), ratio);
		col = vec4(texture(skybox, R).rgb, 1.0);
	}
	else
	{
		
		vec3 specularColor = vec3(texture(material.specular[0], texCoords));

		vec3 norm = normalize(Normal);	
		vec3 viewDir = normalize(cameraPos - FragPos);

		if(!(bPointLights || bDirectionalLight || bSpotLight))
			col = vec4(diffuseColor, 1.0);

		
		//Directional light
		if(bDirectionalLight)
		{
			vec3 dirLigtColor = CalcDirLight(norm, viewDir, diffuseColor, specularColor);
			col += vec4(dirLigtColor, 1.0);
		}

		//Point Light
		if(bPointLights)
		{			
			for(int lightId = 0; lightId < NUM_POINT_LIGHTS; lightId++)
			{
				vec3 pointLightColor = CalcPointLight(lightId, norm, viewDir, diffuseColor, specularColor);
				col += vec4(pointLightColor, 1.0);
			}	
		}

		//Spot Light
		if(bSpotLight)
		{
			vec3 spotLightColor = CalcSpotLight(norm, viewDir, diffuseColor, specularColor);
			col += vec4(spotLightColor, 1.0);	
		}
		
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
	vec3 directedLightDir = normalize(vec3(-directionalLight.direction));
	
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

	
    float shadow = 0.0;
	// calculate shadow
	if(bShadows)
		shadow = DirLightShadowCalculation(normal, directedLightDir);

    // комбинируем результаты
    vec3 ambient  = vec3(directionalLight.ambient)  * diffuseColor;
    vec3 diffuse  = vec3(directionalLight.diffuse)  * diff * diffuseColor * (1.0 - shadow);
    vec3 specular = vec3(directionalLight.specular) * spec * specularColor * (1.0 - shadow);

	

    return (ambient + diffuse + specular);
}

vec3 CalcPointLight(int lightId, vec3 normal, vec3 viewDir, vec3 diffuseColor, vec3 specularColor)
{
	if(!pointLights[lightId].bEnabled)
		return vec3(0.f);
	
	/*Point Light Fading*/
	float dist = length(vec3(pointLights[lightId].position) - FragPos);
	float attenuation = 1.0 / (pointLights[lightId].constant + pointLights[lightId].linear * dist + 
    		    pointLights[lightId].quadratic * (dist * dist));

	vec3 pointLightDir = normalize(vec3(pointLights[lightId].position) - FragPos);	
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
    
	float shadow = 0.0;
	// calculate shadow
	if(bShadows)
		shadow = PointLightShadowCalculation(normal, pointLightDir, lightId);

    // комбинируем результаты
    vec3 ambient  = vec3(pointLights[lightId].ambient)  * diffuseColor;
    vec3 diffuse  = vec3(pointLights[lightId].diffuse)  * diff * diffuseColor * (1.0 - shadow);
    vec3 specular = vec3(pointLights[lightId].specular) * spec * specularColor * (1.0 - shadow);
    ambient  *= attenuation;
    diffuse  *= attenuation;
    specular *= attenuation;
    return (ambient + diffuse + specular);
} 

vec3 CalcSpotLight(vec3 normal, vec3 viewDir, vec3 diffuseColor, vec3 specularColor)
{		
	vec3 spotLightDir = normalize(cameraPos - FragPos);/*projectedLightDir == viewDir*/
	
	/*projected light coeffs*/
	float theta = dot(spotLightDir, normalize(vec3(-spotLight.direction)));
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

	vec3 ambient  = vec3(spotLight.ambient) * diffuseColor;
    vec3 diffuse  = vec3(spotLight.diffuse) * spotDiff * diffuseColor;
    vec3 specular = vec3(spotLight.specular) * spotSpec * specularColor;

    return (ambient + diffuse + specular);
}

float DirLightShadowCalculation(vec3 normal, vec3 lightDir)
{
	// perform perspective divide
    vec3 projCoords = FragPosDirLightSpace.xyz / FragPosDirLightSpace.w;

	//transform coords from [-1, 1] to [0, 1] range
	projCoords = projCoords * 0.5 + 0.5;

	// get closest depth value from light's perspective (using [0,1] range fragPosLight as coords)
    float closestDepth = texture(dirLight_ShadowMap, projCoords.xy).r; 
	//float closestDepth = texture(dirLight_ShadowMap, vec2(0.0)).r; 
    // get depth of current fragment from light's perspective
    float currentDepth = projCoords.z;

	float bias = max(0.05 * (1.0 - dot(normal, lightDir)), 0.005);
    // check whether current frag pos is in shadow

	float shadow  = 0.0;

	if(projCoords.z <= 1.0)
	{
		vec2 texelSize = 1.0 / textureSize(dirLight_ShadowMap, 0);
		for(int x = -1; x <= 1; ++x)
		{
			for(int y = -1; y <= 1; ++y)
			{
				float pcfDepth = texture(dirLight_ShadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
				shadow += currentDepth - bias > pcfDepth  ? 1.0 : 0.0;
			}
		}
		shadow /= 9.0;		
	}

    return shadow;
}

float PointLightShadowCalculation(vec3 normal, vec3 lightDir, int lightId)
{
	//vector between FragPos and light position
	vec3 fragToLight = FragPos - vec3(pointLights[lightId].position);    

	//calculated vector is using for cubemap sampling
    float closestDepth = texture(pointLight_ShadowMaps[lightId], fragToLight).r;
    //transfer linear depth value from [0, 1] range into [0, far_plane] (origin range)
    closestDepth *= far_plane;
	//get linear depth for current fragment as distance from fragment to light position
    float currentDepth = length(fragToLight);
	
	float bias = max(0.05 * (1.0 - dot(normal, lightDir)), 0.005);

	float shadow = 0.0;

	int samples  = 20;
	float viewDistance = length(cameraPos - FragPos);
	float diskRadius = (1.0 + (viewDistance / far_plane)) / 25.0;  
	for(int i = 0; i < samples; ++i)
	{
		float closestDepth = texture(pointLight_ShadowMaps[lightId], fragToLight + sampleOffsetDirections[i] * diskRadius).r;
		closestDepth *= far_plane;   // обратное преобразование из диапазона [0;1]
		if(currentDepth - bias > closestDepth)
			shadow += 1.0;
	}
	shadow /= float(samples);

    //float shadow = currentDepth -  bias > closestDepth ? 1.0 : 0.0;

    return shadow;
}