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
};

layout (std140) uniform Matrices
{
	mat4 view;
    mat4 projection;    
};

out VS_OUT {
    vec3 gs_normal;

	vec2 fs_texCoords;
	vec3 fs_normal;
	vec3 fs_position;

	vec3 fs_fragPos;

} vs_out;

void main()
{
	if(!bShowNormals)
	{
		vs_out.fs_normal = mat3(transpose(inverse(model))) * normal;
		vs_out.fs_position = vec3(model * vec4(position, 1.0));	
		vs_out.fs_texCoords = texCoords;	
	}
	else
	{
		mat3 normalMatrix = mat3(transpose(inverse(view * model)));
		vs_out.gs_normal = normalize(vec3(projection * vec4(normalMatrix * normal, 0.0)));		
	}

	vs_out.fs_fragPos = vec3(model * vec4(position, 1.0f));

	if(bInstancing)
		gl_Position = projection * view * instanceModelMatrix * vec4(position, 1.0);
	else
		gl_Position = projection * view * model * vec4(position, 1.0);
}