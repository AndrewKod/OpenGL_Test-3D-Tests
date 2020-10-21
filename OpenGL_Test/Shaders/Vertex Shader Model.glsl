#version 330 core

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 color;
layout (location = 2) in vec2 texCoords;
layout (location = 3) in vec3 normal;

layout (location = 4) in vec3 tangent;
layout (location = 5) in vec3 bitangent;

//instanced arrays
layout (location = 6) in mat4 instanceModelMatrix;

//out vec2 TexCoords;
//out vec3 Normal;
//out vec3 Position;

uniform mat4 model;
//uniform mat4 view;
//uniform mat4 projection;

uniform bool bInstancing;
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
	
	bool bUseNormalMap;
};

layout (std140) uniform Matrices
{
	mat4 view;
    mat4 projection;    
};

#define NUM_POINT_LIGHTS 16

out VS_OUT {
    vec3 gs_normal;

	vec2 fs_texCoords;
	vec3 fs_normal;
	vec3 fs_position;

	vec3 fs_fragPos;	

	vec4 fs_fragPosDirLightSpace;

	vec3 fs_tanFragPos;
	vec3 fs_tanCameraPos;
	vec3 fs_tanPointLightPositions[NUM_POINT_LIGHTS];
	vec3 fs_tanDirLightDirection;
	vec4 fs_tanFragPosDirLightSpace;
	vec3 fs_tanSpotLightDirection;

} vs_out;

//Shadows
uniform mat4 dirLightSpaceMatrix;

uniform vec3 cameraPos;

layout (std140) uniform PointLightPositions
{
	vec4 pointLightPositions[NUM_POINT_LIGHTS];
};
layout (std140) uniform DirLightDirection
{
	vec4 dirLightDirection;
};
layout (std140) uniform SpotLightDirection
{
	vec4 spotLightDirection;
};

void main()
{
	if(!bShowNormals)
	{
		
		vs_out.fs_normal = mat3(transpose(inverse(bInstancing ? instanceModelMatrix : model))) * normal;
		vs_out.fs_position = vec3((bInstancing ? instanceModelMatrix : model) * vec4(position, 1.0));		
		
		vs_out.fs_texCoords = texCoords;	
	}
	else
	{
		mat3 normalMatrix = mat3(transpose(inverse(view * model)));
		vs_out.gs_normal = normalize(vec3(projection * vec4(normalMatrix * normal, 0.0)));		
	}
	
	vs_out.fs_fragPos = vec3((bInstancing ? instanceModelMatrix : model) * vec4(position, 1.0f));//1.0 for correct lightning	
	
	vs_out.fs_fragPosDirLightSpace = dirLightSpaceMatrix * vec4(vs_out.fs_fragPos, 1.0);

	gl_Position = projection * view * (bInstancing ? instanceModelMatrix : model) * vec4(position, 1.0);

	////////////////NORMAL MAP/////////////////
	vec3 T = normalize(vec3((bInstancing ? instanceModelMatrix : model) * vec4(tangent,   0.0)));
	vec3 B = normalize(vec3((bInstancing ? instanceModelMatrix : model) * vec4(bitangent, 0.0)));
	vec3 N = normalize(vec3((bInstancing ? instanceModelMatrix : model) * vec4(normal,    0.0)));
	mat3 TBN = transpose(mat3(T, B, N));
    vs_out.fs_tanCameraPos  = TBN * cameraPos;
    vs_out.fs_tanFragPos  = TBN * vec3((bInstancing ? instanceModelMatrix : model) * vec4(position, 1.0));//1.0 for correct lightning	
	for(int i = 0; i < NUM_POINT_LIGHTS; i++)
	{
		vs_out.fs_tanPointLightPositions[i] = TBN * vec3(pointLightPositions[i]);
	}

	vs_out.fs_tanDirLightDirection = TBN * vec3(dirLightDirection);

	//vs_out.fs_tanFragPosDirLightSpace = mat4(TBN) * vs_out.fs_fragPosDirLightSpace;

	vs_out.fs_tanSpotLightDirection = TBN * vec3(spotLightDirection);
}