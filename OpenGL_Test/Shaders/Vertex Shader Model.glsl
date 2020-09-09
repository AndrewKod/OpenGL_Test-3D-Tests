#version 330 core

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 color;
layout (location = 2) in vec2 texCoords;
layout (location = 3) in vec3 normal;

layout (location = 4) in vec3 tangent;
layout (location = 5) in vec3 bitangent;

//out vec2 TexCoords;
//out vec3 Normal;
//out vec3 Position;

uniform mat4 model;
//uniform mat4 view;
//uniform mat4 projection;

uniform bool bShowNormals = false;

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
		//vs_out.gs_normal = normalize(vec3(projection * vec4(normalMatrix * normal, 0.0)));
		vs_out.gs_normal =vec3(projection * vec4(normalMatrix * normal, 0.0));
		//vs_out.gs_normal = normalize(vec3(projection * view * model * vec4(normal, 0.0)));
	}

    gl_Position = projection * view * model * vec4(position, 1.0);
	
}