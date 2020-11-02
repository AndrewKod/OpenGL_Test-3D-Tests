#version 330 core

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 color;
layout (location = 2) in vec2 texCoords;
layout (location = 3) in vec3 normal;

layout (location = 4) in vec3 tangent;
layout (location = 5) in vec3 bitangent;

//instanced arrays
layout (location = 6) in mat4 instanceModelMatrix;

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

void main()
{
	gl_Position = projection * view * instanceModelMatrix * vec4(position, 1.0);

	vs_out.normal = mat3(transpose(inverse(instanceModelMatrix))) * normal;			
		
	vs_out.texCoords = texCoords;	

	//vs_out.fragPos = vec3(instanceModelMatrix * vec4(position, 1.0f));//1.0 for correct lightning	

	vs_out.tangent = normalize(vec3(instanceModelMatrix * vec4(tangent, 1.0f)));
	vs_out.bitangent = normalize(vec3(instanceModelMatrix * vec4(bitangent, 1.0f)));	

	mat3 TBN = transpose(mat3(vs_out.tangent, vs_out.bitangent, vs_out.normal));
    vs_out.fragPos  = TBN * vec3(instanceModelMatrix * vec4(position, 1.0));//1.0 for correct lightning	
	
	
}