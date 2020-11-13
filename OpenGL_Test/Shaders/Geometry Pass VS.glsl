#version 330 core

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 color;
layout (location = 2) in vec2 texCoords;
layout (location = 3) in vec3 normal;

layout (location = 4) in vec3 tangent;
layout (location = 5) in vec3 bitangent;

//instanced arrays
layout (location = 6) in mat4 instanceModelMatrix;//mat4 takes 4 locations
layout (location = 10) in mat4 instanceNormalMatrix;


out VS_OUT {
    
	vec2 texCoords;
	vec3 normal;
	
	vec3 fragPos;	
	
	vec3 tangent;
	vec3 bitangent;
	

} vs_out;

layout (std140) uniform Matrices
{
	mat4 view;
    mat4 projection;    
};

bool bViewSpace = true;

void main()
{
	

	vs_out.texCoords = texCoords;
	mat4 modelMatrix;
	if(bViewSpace)
		modelMatrix = view * instanceModelMatrix;
	else
		modelMatrix = instanceModelMatrix;

	mat4 normalMatrix = transpose(inverse(modelMatrix));

	vs_out.normal = normalize(vec3(normalMatrix * vec4(normal, 1.0)));			
	vs_out.tangent = normalize(vec3(normalMatrix * vec4(tangent, 1.0)));
	vs_out.bitangent = normalize(vec3(normalMatrix * vec4(bitangent, 1.0)));	

	//mat3 TBN = transpose(mat3(vs_out.tangent, vs_out.bitangent, vs_out.normal));

	
    vs_out.fragPos  = (vec3(modelMatrix * vec4(position, 1.0)));//1.0 for correct lightning	
	
	gl_Position = projection * modelMatrix * vec4(position, 1.0);
}